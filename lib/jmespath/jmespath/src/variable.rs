//! Module for JMESPath runtime variables.

use serde::de::IntoDeserializer;
use serde::*;
use serde_json::error::Error;
use serde_json::value::Value;
use std::cmp::{max, Ordering};
use std::collections::BTreeMap;
use std::fmt;
use std::iter::Iterator;
use std::string::ToString;
use std::vec;

use crate::ast::{Ast, Comparator};
use crate::ToJmespath;
use crate::{JmespathError, Rcvar};
use serde_json::Number;
use std::convert::TryFrom;

/// JMESPath types.
#[derive(Debug, PartialEq, PartialOrd, Eq, Ord)]
pub enum JmespathType {
    Null,
    String,
    Number,
    Boolean,
    Array,
    Object,
    Expref,
}

impl fmt::Display for JmespathType {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        write!(
            fmt,
            "{}",
            match *self {
                JmespathType::Null => "null",
                JmespathType::String => "string",
                JmespathType::Number => "number",
                JmespathType::Boolean => "boolean",
                JmespathType::Array => "array",
                JmespathType::Object => "object",
                JmespathType::Expref => "expref",
            }
        )
    }
}

/// JMESPath variable.
#[derive(Clone, Debug)]
pub enum Variable {
    Null,
    String(String),
    Bool(bool),
    Number(Number),
    Array(Vec<Rcvar>),
    Object(BTreeMap<String, Rcvar>),
    Expref(Ast),
}

impl Eq for Variable {}

/// Compares two floats for equality.
///
/// Allows for equivalence of floating point numbers like
/// 0.7100000000000002 and 0.71.
///
/// Based on http://stackoverflow.com/a/4915891
fn float_eq(a: f64, b: f64) -> bool {
    use std::f64;
    let abs_a = a.abs();
    let abs_b = b.abs();
    let diff = (a - b).abs();
    if a == b {
        true
    } else if !a.is_normal() || !b.is_normal() {
        // a or b is zero or both are extremely close to it
        // relative error is less meaningful here.
        diff < (f64::EPSILON * f64::MIN_POSITIVE)
    } else {
        // use relative error.
        diff / (abs_a + abs_b) < f64::EPSILON
    }
}

/// Implement PartialEq for looser floating point comparisons.
impl PartialEq for Variable {
    fn eq(&self, other: &Variable) -> bool {
        if self.get_type() != other.get_type() {
            false
        } else {
            match self {
                Variable::Number(a) => {
                    if let (Some(a), Some(b)) = (a.as_f64(), other.as_number()) {
                        float_eq(a, b)
                    } else {
                        false
                    }
                }
                Variable::String(ref s) => Some(s) == other.as_string(),
                Variable::Bool(b) => Some(*b) == other.as_boolean(),
                Variable::Array(ref a) => Some(a) == other.as_array(),
                Variable::Object(ref o) => Some(o) == other.as_object(),
                Variable::Expref(ref e) => Some(e) == other.as_expref(),
                Variable::Null => true,
            }
        }
    }
}

/// Implement PartialOrd so that Ast can be in the PartialOrd of Variable.
impl PartialOrd<Variable> for Variable {
    fn partial_cmp(&self, other: &Variable) -> Option<Ordering> {
        Some(self.cmp(other))
    }

    fn lt(&self, other: &Variable) -> bool {
        self.cmp(other) == Ordering::Less
    }

    fn le(&self, other: &Variable) -> bool {
        let ordering = self.cmp(other);
        ordering == Ordering::Equal || ordering == Ordering::Less
    }

    fn gt(&self, other: &Variable) -> bool {
        self.cmp(other) == Ordering::Greater
    }

    fn ge(&self, other: &Variable) -> bool {
        let ordering = self.cmp(other);
        ordering == Ordering::Equal || ordering == Ordering::Greater
    }
}

impl Ord for Variable {
    fn cmp(&self, other: &Self) -> Ordering {
        let var_type = self.get_type();
        // Variables of different types are considered equal.
        if var_type != other.get_type() {
            Ordering::Equal
        } else {
            match var_type {
                JmespathType::String => {
                    if let (Some(a), Some(b)) = (self.as_string(), other.as_string()) {
                        a.cmp(b)
                    } else {
                        Ordering::Equal
                    }
                }
                JmespathType::Number => {
                    if let (Some(a), Some(b)) = (self.as_number(), other.as_number()) {
                        a.partial_cmp(&b).unwrap_or(Ordering::Less)
                    } else {
                        Ordering::Equal
                    }
                }
                _ => Ordering::Equal,
            }
        }
    }
}

/// Write the JSON representation of a value, converting expref to a JSON
/// string containing the debug dump of the expref variable.
impl fmt::Display for Variable {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        write!(
            fmt,
            "{}",
            serde_json::to_string(self)
                .unwrap_or_else(|err| format!("unable to stringify Variable. Err: {}", err))
        )
    }
}

/// Generic way of converting a Map in a Value to a Variable.
fn convert_map<'a, T>(value: T) -> Result<Variable, JmespathError>
where
    T: Iterator<Item = (&'a String, &'a Value)>,
{
    let mut map: BTreeMap<String, Rcvar> = BTreeMap::new();
    for kvp in value {
        map.insert(kvp.0.to_owned(), kvp.1.to_jmespath()?);
    }
    Ok(Variable::Object(map))
}

/// Convert a borrowed Value to a Variable.
impl<'a> TryFrom<&'a Value> for Variable {
    type Error = JmespathError;

    fn try_from(value: &'a Value) -> Result<Self, Self::Error> {
        let var = match *value {
            Value::String(ref s) => Variable::String(s.to_owned()),
            Value::Null => Variable::Null,
            Value::Bool(b) => Variable::Bool(b),
            Value::Number(ref n) => Variable::Number(n.clone()),
            Value::Object(ref values) => convert_map(values.iter())?,
            Value::Array(ref values) => Variable::Array(
                values
                    .iter()
                    .map(|v| v.to_jmespath())
                    .collect::<Result<_, JmespathError>>()?,
            ),
        };
        Ok(var)
    }
}

/// Slightly optimized method for converting from an owned Value.
impl TryFrom<Value> for Variable {
    type Error = JmespathError;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        let var = match value {
            Value::String(s) => Variable::String(s),
            Value::Null => Variable::Null,
            Value::Bool(b) => Variable::Bool(b),
            Value::Number(n) => Variable::Number(n),
            Value::Object(ref values) => convert_map(values.iter())?,
            Value::Array(mut values) => Variable::Array(
                values
                    .drain(..)
                    .map(|v| v.to_jmespath())
                    .collect::<Result<_, JmespathError>>()?,
            ),
        };
        Ok(var)
    }
}

impl Variable {
    /// Shortcut function to encode a `T` into a JMESPath `Variable`
    pub fn from_serializable<T>(value: T) -> Result<Variable, JmespathError>
    where
        T: ser::Serialize,
    {
        Ok(to_variable(value)?)
    }

    /// Create a JMESPath Variable from a JSON encoded string.
    pub fn from_json(s: &str) -> Result<Self, String> {
        serde_json::from_str::<Variable>(s).map_err(|e| e.to_string())
    }

    /// Returns true if the Variable is an Array. Returns false otherwise.
    pub fn is_array(&self) -> bool {
        self.as_array().is_some()
    }

    /// If the Variable value is an Array, returns the associated vector.
    /// Returns None otherwise.
    pub fn as_array(&self) -> Option<&Vec<Rcvar>> {
        match self {
            Variable::Array(array) => Some(array),
            _ => None,
        }
    }

    /// Returns true if the value is an Object.
    pub fn is_object(&self) -> bool {
        self.as_object().is_some()
    }

    /// If the value is an Object, returns the associated BTreeMap.
    /// Returns None otherwise.
    pub fn as_object(&self) -> Option<&BTreeMap<String, Rcvar>> {
        match self {
            Variable::Object(map) => Some(map),
            _ => None,
        }
    }

    /// Returns true if the value is a String. Returns false otherwise.
    pub fn is_string(&self) -> bool {
        self.as_string().is_some()
    }

    /// If the value is a String, returns the associated str.
    /// Returns None otherwise.
    pub fn as_string(&self) -> Option<&String> {
        match self {
            Variable::String(ref s) => Some(s),
            _ => None,
        }
    }

    /// Returns true if the value is a Number. Returns false otherwise.
    pub fn is_number(&self) -> bool {
        matches!(self, Variable::Number(_))
    }

    /// If the value is a number, return or cast it to a f64.
    /// Returns None otherwise.
    pub fn as_number(&self) -> Option<f64> {
        match self {
            Variable::Number(f) => f.as_f64(),
            _ => None,
        }
    }

    /// Returns true if the value is a Boolean. Returns false otherwise.
    pub fn is_boolean(&self) -> bool {
        self.as_boolean().is_some()
    }

    /// If the value is a Boolean, returns the associated bool.
    /// Returns None otherwise.
    pub fn as_boolean(&self) -> Option<bool> {
        match self {
            Variable::Bool(b) => Some(*b),
            _ => None,
        }
    }

    /// Returns true if the value is a Null. Returns false otherwise.
    pub fn is_null(&self) -> bool {
        self.as_null().is_some()
    }

    /// If the value is a Null, returns ().
    /// Returns None otherwise.
    pub fn as_null(&self) -> Option<()> {
        match self {
            Variable::Null => Some(()),
            _ => None,
        }
    }

    /// Returns true if the value is an expression reference.
    /// Returns false otherwise.
    pub fn is_expref(&self) -> bool {
        self.as_expref().is_some()
    }

    /// If the value is an expression reference, returns the associated Ast node.
    /// Returns None otherwise.
    pub fn as_expref(&self) -> Option<&Ast> {
        match *self {
            Variable::Expref(ref ast) => Some(ast),
            _ => None,
        }
    }

    /// If the value is an object, returns the value associated with the provided key.
    /// Otherwise, returns Null.
    #[inline]
    pub fn get_field(&self, key: &str) -> Rcvar {
        if let Variable::Object(ref map) = self {
            if let Some(result) = map.get(key) {
                return result.clone();
            }
        }
        Rcvar::new(Variable::Null)
    }

    /// If the value is an array, then gets an array value by index. Otherwise returns Null.
    #[inline]
    pub fn get_index(&self, index: usize) -> Rcvar {
        if let Variable::Array(ref array) = self {
            if let Some(result) = array.get(index) {
                return result.clone();
            }
        }
        Rcvar::new(Variable::Null)
    }

    /// Retrieves an index from the end of an array.
    ///
    /// Returns Null if not an array or if the index is not present.
    /// The formula for determining the index position is length - index (i.e., an
    /// index of 0 or 1 is treated as the end of the array).
    pub fn get_negative_index(&self, index: usize) -> Rcvar {
        if let Variable::Array(ref array) = self {
            let adjusted_index = max(index, 1);
            if array.len() >= adjusted_index {
                return array[array.len() - adjusted_index].clone();
            }
        }
        Rcvar::new(Variable::Null)
    }

    /// Returns true or false based on if the Variable value is considered truthy.
    pub fn is_truthy(&self) -> bool {
        match self {
            Variable::Bool(b) => *b,
            Variable::String(ref s) => !s.is_empty(),
            Variable::Array(ref a) => !a.is_empty(),
            Variable::Object(ref o) => !o.is_empty(),
            Variable::Number(_) => true,
            _ => false,
        }
    }

    /// Returns the JMESPath type name of a Variable value.
    pub fn get_type(&self) -> JmespathType {
        match *self {
            Variable::Bool(_) => JmespathType::Boolean,
            Variable::String(_) => JmespathType::String,
            Variable::Number(_) => JmespathType::Number,
            Variable::Array(_) => JmespathType::Array,
            Variable::Object(_) => JmespathType::Object,
            Variable::Null => JmespathType::Null,
            Variable::Expref(_) => JmespathType::Expref,
        }
    }

    /// Compares two Variable values using a comparator.
    pub fn compare(&self, cmp: &Comparator, value: &Variable) -> Option<bool> {
        // Ordering requires numeric values.
        if !(self.is_number() && value.is_number()
            || *cmp == Comparator::NotEqual
            || *cmp == Comparator::Equal)
        {
            return None;
        }
        match *cmp {
            Comparator::Equal => Some(*self == *value),
            Comparator::NotEqual => Some(*self != *value),
            Comparator::LessThan => Some(*self < *value),
            Comparator::LessThanEqual => Some(*self <= *value),
            Comparator::GreaterThan => Some(*self > *value),
            Comparator::GreaterThanEqual => Some(*self >= *value),
        }
    }

    /// Returns a slice of the variable if the variable is an array.
    pub fn slice(&self, start: Option<i32>, stop: Option<i32>, step: i32) -> Option<Vec<Rcvar>> {
        self.as_array().map(|a| slice(a, start, stop, step))
    }
}

impl Variable {
    fn unexpected(&self) -> de::Unexpected<'_> {
        match self {
            Variable::Null => de::Unexpected::Unit,
            Variable::Bool(b) => de::Unexpected::Bool(*b),
            Variable::Number(_) => de::Unexpected::Other("number"),
            Variable::String(s) => de::Unexpected::Str(s),
            Variable::Array(_) => de::Unexpected::Seq,
            Variable::Object(_) => de::Unexpected::Map,
            Variable::Expref(_) => de::Unexpected::Other("expression"),
        }
    }
}

// ------------------------------------------
// Variable slicing implementation
// ------------------------------------------

fn slice(array: &[Rcvar], start: Option<i32>, stop: Option<i32>, step: i32) -> Vec<Rcvar> {
    let mut result = vec![];
    let len = array.len() as i32;
    if len == 0 {
        return result;
    }
    let a: i32 = match start {
        Some(starting_index) => adjust_slice_endpoint(len, starting_index, step),
        _ if step < 0 => len - 1,
        _ => 0,
    };
    let b: i32 = match stop {
        Some(ending_index) => adjust_slice_endpoint(len, ending_index, step),
        _ if step < 0 => -1,
        _ => len,
    };
    let mut i = a;
    if step > 0 {
        while i < b {
            result.push(array[i as usize].clone());
            i += step;
        }
    } else {
        while i > b {
            result.push(array[i as usize].clone());
            i += step;
        }
    }
    result
}

#[inline]
fn adjust_slice_endpoint(len: i32, mut endpoint: i32, step: i32) -> i32 {
    if endpoint < 0 {
        endpoint += len;
        if endpoint >= 0 {
            endpoint
        } else if step < 0 {
            -1
        } else {
            0
        }
    } else if endpoint < len {
        endpoint
    } else if step < 0 {
        len - 1
    } else {
        len
    }
}

// Serde Variable deserialization
// ------------------------------------------
// Below begins the amazingly complicated code needed to implement
// serde serialization and deserialization for Variable. This code
// is ported from the serde_json::Value implementation and adapted
// for Variable and Rcvar. There are a few instances of
// `(*self).clone` that I don't like, but I'm not sure how to work
// around it.

/// Shortcut function to encode a `T` into a JMESPath `Variable`
fn to_variable<T>(value: T) -> Result<Variable, Error>
where
    T: ser::Serialize,
{
    value.serialize(Serializer)
}

/// Serde deserialization for Variable
impl<'de> de::Deserialize<'de> for Variable {
    #[inline]
    fn deserialize<D>(deserializer: D) -> Result<Variable, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        struct VariableVisitor;

        impl<'de> de::Visitor<'de> for VariableVisitor {
            type Value = Variable;

            fn expecting(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
                formatter.write_str("any valid JMESPath variable")
            }

            #[inline]
            fn visit_bool<E>(self, value: bool) -> Result<Variable, E> {
                Ok(Variable::Bool(value))
            }

            #[inline]
            fn visit_i64<E>(self, value: i64) -> Result<Variable, E> {
                Ok(Variable::Number(value.into()))
            }

            #[inline]
            fn visit_u64<E>(self, value: u64) -> Result<Variable, E> {
                Ok(Variable::Number(value.into()))
            }

            #[inline]
            fn visit_f64<E>(self, value: f64) -> Result<Variable, E> {
                Ok(Number::from_f64(value).map_or(Variable::Null, Variable::Number))
            }

            #[inline]
            fn visit_str<E>(self, value: &str) -> Result<Variable, E>
            where
                E: de::Error,
            {
                self.visit_string(String::from(value))
            }

            #[inline]
            fn visit_string<E>(self, value: String) -> Result<Variable, E> {
                Ok(Variable::String(value))
            }

            #[inline]
            fn visit_none<E>(self) -> Result<Variable, E> {
                Ok(Variable::Null)
            }

            #[inline]
            fn visit_some<D>(self, deserializer: D) -> Result<Variable, D::Error>
            where
                D: de::Deserializer<'de>,
            {
                de::Deserialize::deserialize(deserializer)
            }

            #[inline]
            fn visit_unit<E>(self) -> Result<Variable, E> {
                Ok(Variable::Null)
            }

            #[inline]
            fn visit_seq<V>(self, mut visitor: V) -> Result<Variable, V::Error>
            where
                V: de::SeqAccess<'de>,
            {
                let mut values = vec![];

                while let Some(elem) = visitor.next_element()? {
                    values.push(elem);
                }

                Ok(Variable::Array(values))
            }

            #[inline]
            fn visit_map<V>(self, mut visitor: V) -> Result<Variable, V::Error>
            where
                V: de::MapAccess<'de>,
            {
                let mut values = BTreeMap::new();

                while let Some((key, value)) = visitor.next_entry()? {
                    values.insert(key, value);
                }

                Ok(Variable::Object(values))
            }
        }

        deserializer.deserialize_any(VariableVisitor)
    }
}

impl<'de> de::Deserializer<'de> for Variable {
    type Error = Error;

    #[inline]
    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Error>
    where
        V: de::Visitor<'de>,
    {
        match self {
            Variable::Null => visitor.visit_unit(),
            Variable::Bool(v) => visitor.visit_bool(v),
            Variable::Number(v) => v.deserialize_any(visitor),
            Variable::String(v) => visitor.visit_string(v),
            Variable::Array(v) => {
                let len = v.len();
                visitor.visit_seq(SeqDeserializer {
                    iter: v.into_iter(),
                    len,
                })
            }
            Variable::Object(v) => visitor.visit_map(MapDeserializer {
                iter: v.into_iter(),
                value: None,
            }),
            Variable::Expref(v) => visitor.visit_string(format!("<expression: {:?}>", v)),
        }
    }

    #[inline]
    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, Error>
    where
        V: de::Visitor<'de>,
    {
        match self {
            Variable::Null => visitor.visit_none(),
            _ => visitor.visit_some(self),
        }
    }

    #[inline]
    fn deserialize_enum<V>(
        self,
        _name: &str,
        _variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Error>
    where
        V: de::Visitor<'de>,
    {
        let (variant, value) = match self {
            Variable::Object(value) => {
                let mut iter = value.into_iter();
                let (variant, value) = match iter.next() {
                    Some(v) => v,
                    None => {
                        return Err(de::Error::invalid_value(
                            de::Unexpected::Map,
                            &"map with a single key",
                        ))
                    }
                };
                // enums are encoded in json as maps with a single key:value pair
                if iter.next().is_some() {
                    return Err(de::Error::invalid_value(
                        de::Unexpected::Map,
                        &"map with a single key",
                    ));
                }
                (variant, Some((*value).clone()))
            }
            Variable::String(variant) => (variant, None),
            other => {
                return Err(de::Error::invalid_type(
                    other.unexpected(),
                    &"string or map",
                ));
            }
        };

        visitor.visit_enum(EnumDeserializer {
            val: value,
            variant,
        })
    }

    #[inline]
    fn deserialize_newtype_struct<V>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: de::Visitor<'de>,
    {
        visitor.visit_newtype_struct(self)
    }

    forward_to_deserialize_any! {
        bool u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 char str string
        unit seq bytes byte_buf map unit_struct tuple_struct struct
        identifier tuple ignored_any
    }
}

struct VariantDeserializer {
    val: Option<Variable>,
}

impl<'de> de::VariantAccess<'de> for VariantDeserializer {
    type Error = Error;

    fn unit_variant(self) -> Result<(), Error> {
        match self.val {
            Some(value) => de::Deserialize::deserialize(value),
            None => Ok(()),
        }
    }

    fn newtype_variant_seed<T>(self, seed: T) -> Result<T::Value, Error>
    where
        T: de::DeserializeSeed<'de>,
    {
        match self.val {
            Some(value) => seed.deserialize(value),
            None => Err(de::Error::invalid_type(
                de::Unexpected::UnitVariant,
                &"newtype variant",
            )),
        }
    }

    fn tuple_variant<V>(self, _len: usize, visitor: V) -> Result<V::Value, Error>
    where
        V: de::Visitor<'de>,
    {
        match self.val {
            Some(Variable::Array(fields)) => de::Deserializer::deserialize_any(
                SeqDeserializer {
                    len: fields.len(),
                    iter: fields.into_iter(),
                },
                visitor,
            ),
            Some(other) => Err(de::Error::invalid_type(
                other.unexpected(),
                &"tuple variant",
            )),
            None => Err(de::Error::invalid_type(
                de::Unexpected::UnitVariant,
                &"tuple variant",
            )),
        }
    }

    fn struct_variant<V>(
        self,
        _fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Error>
    where
        V: de::Visitor<'de>,
    {
        match self.val {
            Some(Variable::Object(fields)) => de::Deserializer::deserialize_any(
                MapDeserializer {
                    iter: fields.into_iter(),
                    value: None,
                },
                visitor,
            ),
            Some(other) => Err(de::Error::invalid_type(
                other.unexpected(),
                &"struct variant",
            )),
            _ => Err(de::Error::invalid_type(
                de::Unexpected::UnitVariant,
                &"struct variant",
            )),
        }
    }
}

struct EnumDeserializer {
    variant: String,
    val: Option<Variable>,
}

impl<'de> de::EnumAccess<'de> for EnumDeserializer {
    type Error = Error;
    type Variant = VariantDeserializer;

    fn variant_seed<V>(self, seed: V) -> Result<(V::Value, VariantDeserializer), Error>
    where
        V: de::DeserializeSeed<'de>,
    {
        let variant = self.variant.into_deserializer();
        let visitor = VariantDeserializer { val: self.val };
        seed.deserialize(variant).map(|v| (v, visitor))
    }
}

struct SeqDeserializer {
    iter: vec::IntoIter<Rcvar>,
    len: usize,
}

impl<'de> de::Deserializer<'de> for SeqDeserializer {
    type Error = Error;

    #[inline]
    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Error>
    where
        V: de::Visitor<'de>,
    {
        if self.len == 0 {
            visitor.visit_unit()
        } else {
            visitor.visit_seq(self)
        }
    }

    forward_to_deserialize_any! {
        bool u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 char str string
        unit option seq bytes byte_buf map unit_struct newtype_struct
        tuple_struct struct identifier tuple enum ignored_any
    }
}

impl<'de> de::SeqAccess<'de> for SeqDeserializer {
    type Error = Error;

    fn next_element_seed<T>(&mut self, seed: T) -> Result<Option<T::Value>, Error>
    where
        T: de::DeserializeSeed<'de>,
    {
        match self.iter.next() {
            Some(value) => seed.deserialize(Variable::clone(&value)).map(Some),
            None => Ok(None),
        }
    }

    fn size_hint(&self) -> Option<usize> {
        match self.iter.size_hint() {
            (lower, Some(upper)) if lower == upper => Some(upper),
            _ => None,
        }
    }
}

struct MapDeserializer {
    iter: <BTreeMap<String, Rcvar> as IntoIterator>::IntoIter,
    value: Option<Variable>,
}

impl<'de> de::MapAccess<'de> for MapDeserializer {
    type Error = Error;

    fn next_key_seed<T>(&mut self, seed: T) -> Result<Option<T::Value>, Error>
    where
        T: de::DeserializeSeed<'de>,
    {
        match self.iter.next() {
            Some((key, value)) => {
                self.value = Some(Variable::clone(&value));
                seed.deserialize(Variable::String(key)).map(Some)
            }
            None => Ok(None),
        }
    }

    fn next_value_seed<T>(&mut self, seed: T) -> Result<T::Value, Error>
    where
        T: de::DeserializeSeed<'de>,
    {
        match self.value.take() {
            Some(value) => seed.deserialize(value),
            None => Err(de::Error::custom("value is missing")),
        }
    }

    fn size_hint(&self) -> Option<usize> {
        match self.iter.size_hint() {
            (lower, Some(upper)) if lower == upper => Some(upper),
            _ => None,
        }
    }
}

impl<'de> de::Deserializer<'de> for MapDeserializer {
    type Error = Error;

    #[inline]
    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Error>
    where
        V: de::Visitor<'de>,
    {
        visitor.visit_map(self)
    }

    forward_to_deserialize_any! {
        bool u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 char str string
        unit option seq bytes byte_buf map unit_struct newtype_struct
        tuple_struct struct identifier tuple enum ignored_any
    }
}

// Serde Variable serialization
impl ser::Serialize for Variable {
    #[inline]
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: ser::Serializer,
    {
        match self {
            Variable::Null => serializer.serialize_unit(),
            Variable::Bool(v) => serializer.serialize_bool(*v),
            Variable::Number(v) => v.serialize(serializer),
            Variable::String(ref v) => serializer.serialize_str(v),
            Variable::Array(ref v) => v.serialize(serializer),
            Variable::Object(ref v) => v.serialize(serializer),
            Variable::Expref(ref e) => serializer.serialize_str(&format!("<expression: {:?}>", e)),
        }
    }
}

/// Create a `serde::Serializer` that serializes a `Serialize`e into a `Variable`.
#[derive(Debug, Default)]
pub struct Serializer;

#[doc(hidden)]
pub struct SeqState(Vec<Rcvar>);

#[doc(hidden)]
pub struct TupleVariantState {
    name: String,
    vec: Vec<Rcvar>,
}

#[doc(hidden)]
pub struct StructVariantState {
    name: String,
    map: BTreeMap<String, Rcvar>,
}

#[doc(hidden)]
pub struct MapState {
    map: BTreeMap<String, Rcvar>,
    next_key: Option<String>,
}

impl ser::Serializer for Serializer {
    type Ok = Variable;
    type Error = Error;

    type SerializeSeq = SeqState;
    type SerializeTuple = SeqState;
    type SerializeTupleStruct = SeqState;
    type SerializeTupleVariant = TupleVariantState;
    type SerializeMap = MapState;
    type SerializeStruct = MapState;
    type SerializeStructVariant = StructVariantState;

    #[inline]
    fn serialize_bool(self, value: bool) -> Result<Variable, Error> {
        Ok(Variable::Bool(value))
    }

    #[inline]
    fn serialize_i8(self, value: i8) -> Result<Variable, Error> {
        Ok(Variable::Number(Number::from(value)))
    }

    #[inline]
    fn serialize_i16(self, value: i16) -> Result<Variable, Error> {
        Ok(Variable::Number(Number::from(value)))
    }

    #[inline]
    fn serialize_i32(self, value: i32) -> Result<Variable, Error> {
        Ok(Variable::Number(Number::from(value)))
    }

    fn serialize_i64(self, value: i64) -> Result<Variable, Error> {
        Ok(Variable::Number(Number::from(value)))
    }

    #[inline]
    fn serialize_u8(self, value: u8) -> Result<Variable, Error> {
        Ok(Variable::Number(Number::from(value)))
    }

    #[inline]
    fn serialize_u16(self, value: u16) -> Result<Variable, Error> {
        Ok(Variable::Number(Number::from(value)))
    }

    #[inline]
    fn serialize_u32(self, value: u32) -> Result<Variable, Error> {
        Ok(Variable::Number(Number::from(value)))
    }

    #[inline]
    fn serialize_u64(self, value: u64) -> Result<Variable, Error> {
        Ok(Variable::Number(Number::from(value)))
    }

    #[inline]
    fn serialize_f32(self, value: f32) -> Result<Variable, Error> {
        self.serialize_f64(value as f64)
    }

    #[inline]
    fn serialize_f64(self, value: f64) -> Result<Variable, Error> {
        Ok(Number::from_f64(value).map_or(Variable::Null, Variable::Number))
    }

    #[inline]
    fn serialize_char(self, value: char) -> Result<Variable, Error> {
        let mut s = String::new();
        s.push(value);
        self.serialize_str(&s)
    }

    #[inline]
    fn serialize_str(self, value: &str) -> Result<Variable, Error> {
        Ok(Variable::String(String::from(value)))
    }

    fn serialize_bytes(self, value: &[u8]) -> Result<Variable, Error> {
        let vec = value
            .iter()
            .map(|&b| Rcvar::new(Variable::Number(Number::from(b))))
            .collect();
        Ok(Variable::Array(vec))
    }

    #[inline]
    fn serialize_unit(self) -> Result<Variable, Error> {
        Ok(Variable::Null)
    }

    #[inline]
    fn serialize_unit_struct(self, _name: &'static str) -> Result<Variable, Error> {
        self.serialize_unit()
    }

    #[inline]
    fn serialize_unit_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
    ) -> Result<Variable, Error> {
        self.serialize_str(variant)
    }

    #[inline]
    fn serialize_newtype_struct<T: ?Sized>(
        self,
        _name: &'static str,
        value: &T,
    ) -> Result<Variable, Error>
    where
        T: ser::Serialize,
    {
        value.serialize(self)
    }

    fn serialize_newtype_variant<T: ?Sized>(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
        value: &T,
    ) -> Result<Variable, Error>
    where
        T: ser::Serialize,
    {
        let mut values = BTreeMap::new();
        values.insert(String::from(variant), Rcvar::new(to_variable(&value)?));
        Ok(Variable::Object(values))
    }

    #[inline]
    fn serialize_none(self) -> Result<Variable, Error> {
        self.serialize_unit()
    }

    #[inline]
    fn serialize_some<V: ?Sized>(self, value: &V) -> Result<Variable, Error>
    where
        V: ser::Serialize,
    {
        value.serialize(self)
    }

    fn serialize_seq(self, len: Option<usize>) -> Result<SeqState, Error> {
        Ok(SeqState(Vec::with_capacity(len.unwrap_or(0))))
    }

    fn serialize_tuple(self, len: usize) -> Result<SeqState, Error> {
        self.serialize_seq(Some(len))
    }

    fn serialize_tuple_struct(self, _name: &'static str, len: usize) -> Result<SeqState, Error> {
        self.serialize_seq(Some(len))
    }

    fn serialize_tuple_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
        len: usize,
    ) -> Result<TupleVariantState, Error> {
        Ok(TupleVariantState {
            name: String::from(variant),
            vec: Vec::with_capacity(len),
        })
    }

    fn serialize_map(self, _len: Option<usize>) -> Result<MapState, Error> {
        Ok(MapState {
            map: BTreeMap::new(),
            next_key: None,
        })
    }

    fn serialize_struct(self, _name: &'static str, len: usize) -> Result<MapState, Error> {
        self.serialize_map(Some(len))
    }

    fn serialize_struct_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
        _len: usize,
    ) -> Result<StructVariantState, Error> {
        Ok(StructVariantState {
            name: String::from(variant),
            map: BTreeMap::new(),
        })
    }
}

impl ser::SerializeSeq for SeqState {
    type Ok = Variable;
    type Error = Error;

    fn serialize_element<T: ?Sized>(&mut self, value: &T) -> Result<(), Error>
    where
        T: ser::Serialize,
    {
        self.0.push(Rcvar::new(to_variable(value)?));
        Ok(())
    }

    fn end(self) -> Result<Variable, Error> {
        Ok(Variable::Array(self.0))
    }
}

impl ser::SerializeTuple for SeqState {
    type Ok = Variable;
    type Error = Error;

    fn serialize_element<T: ?Sized>(&mut self, value: &T) -> Result<(), Error>
    where
        T: ser::Serialize,
    {
        ser::SerializeSeq::serialize_element(self, value)
    }

    fn end(self) -> Result<Variable, Error> {
        ser::SerializeSeq::end(self)
    }
}

impl ser::SerializeTupleStruct for SeqState {
    type Ok = Variable;
    type Error = Error;

    fn serialize_field<T: ?Sized>(&mut self, value: &T) -> Result<(), Error>
    where
        T: ser::Serialize,
    {
        ser::SerializeSeq::serialize_element(self, value)
    }

    fn end(self) -> Result<Variable, Error> {
        ser::SerializeSeq::end(self)
    }
}

impl ser::SerializeTupleVariant for TupleVariantState {
    type Ok = Variable;
    type Error = Error;

    fn serialize_field<T: ?Sized>(&mut self, value: &T) -> Result<(), Error>
    where
        T: ser::Serialize,
    {
        self.vec.push(Rcvar::new(to_variable(&value)?));
        Ok(())
    }

    fn end(self) -> Result<Variable, Error> {
        let mut object = BTreeMap::new();
        object.insert(self.name, Rcvar::new(Variable::Array(self.vec)));
        Ok(Variable::Object(object))
    }
}

impl ser::SerializeMap for MapState {
    type Ok = Variable;
    type Error = Error;

    fn serialize_key<T: ?Sized>(&mut self, key: &T) -> Result<(), Error>
    where
        T: ser::Serialize,
    {
        match to_variable(key)? {
            Variable::String(s) => self.next_key = Some(s),
            _ => return Err(de::Error::custom("KeyMustBeAString")),
        };
        Ok(())
    }

    fn serialize_value<T: ?Sized>(&mut self, value: &T) -> Result<(), Error>
    where
        T: ser::Serialize,
    {
        let key = self
            .next_key
            .take()
            .expect("serialize_value called before serialize_key");
        self.map.insert(key, Rcvar::new(to_variable(value)?));
        Ok(())
    }

    fn end(self) -> Result<Variable, Error> {
        Ok(Variable::Object(self.map))
    }
}

impl ser::SerializeStruct for MapState {
    type Ok = Variable;
    type Error = Error;

    fn serialize_field<T: ?Sized>(&mut self, key: &'static str, value: &T) -> Result<(), Error>
    where
        T: ser::Serialize,
    {
        ser::SerializeMap::serialize_key(self, key)?;
        ser::SerializeMap::serialize_value(self, value)
    }

    fn end(self) -> Result<Variable, Error> {
        ser::SerializeMap::end(self)
    }
}

impl ser::SerializeStructVariant for StructVariantState {
    type Ok = Variable;
    type Error = Error;

    fn serialize_field<T: ?Sized>(&mut self, key: &'static str, value: &T) -> Result<(), Error>
    where
        T: ser::Serialize,
    {
        self.map
            .insert(String::from(key), Rcvar::new(to_variable(value)?));
        Ok(())
    }

    fn end(self) -> Result<Variable, Error> {
        let mut object = BTreeMap::new();
        object.insert(self.name, Rcvar::new(Variable::Object(self.map)));
        Ok(Variable::Object(object))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ast::{Ast, Comparator};
    use crate::Rcvar;
    use serde_json::{self, Number, Value};
    use std::collections::BTreeMap;

    #[test]
    fn creates_variable_from_str() {
        assert_eq!(Ok(Variable::Bool(true)), Variable::from_json("true"));
        assert_eq!(
            Err("expected value at line 1 column 1".to_string()),
            Variable::from_json("abc")
        );
    }

    #[test]
    fn test_determines_types() {
        assert_eq!(
            JmespathType::Object,
            Variable::from_json(&"{\"foo\": \"bar\"}")
                .unwrap()
                .get_type()
        );
        assert_eq!(
            JmespathType::Array,
            Variable::from_json(&"[\"foo\"]").unwrap().get_type()
        );
        assert_eq!(JmespathType::Null, Variable::Null.get_type());
        assert_eq!(JmespathType::Boolean, Variable::Bool(true).get_type());
        assert_eq!(
            JmespathType::String,
            Variable::String("foo".to_string()).get_type()
        );
        assert_eq!(
            JmespathType::Number,
            Variable::Number(Number::from_f64(1.0).unwrap()).get_type()
        );
    }

    #[test]
    fn test_is_truthy() {
        assert_eq!(
            true,
            Variable::from_json(&"{\"foo\": \"bar\"}")
                .unwrap()
                .is_truthy()
        );
        assert_eq!(false, Variable::from_json(&"{}").unwrap().is_truthy());
        assert_eq!(true, Variable::from_json(&"[\"foo\"]").unwrap().is_truthy());
        assert_eq!(false, Variable::from_json(&"[]").unwrap().is_truthy());
        assert_eq!(false, Variable::Null.is_truthy());
        assert_eq!(true, Variable::Bool(true).is_truthy());
        assert_eq!(false, Variable::Bool(false).is_truthy());
        assert_eq!(true, Variable::String("foo".to_string()).is_truthy());
        assert_eq!(false, Variable::String("".to_string()).is_truthy());
        assert_eq!(
            true,
            Variable::Number(Number::from_f64(10.0).unwrap()).is_truthy()
        );
        assert_eq!(
            true,
            Variable::Number(Number::from_f64(0.0).unwrap()).is_truthy()
        );
    }

    #[test]
    fn test_eq_ne_compare() {
        let l = Variable::String("foo".to_string());
        let r = Variable::String("foo".to_string());
        assert_eq!(Some(true), l.compare(&Comparator::Equal, &r));
        assert_eq!(Some(false), l.compare(&Comparator::NotEqual, &r));
    }

    #[test]
    fn test_compare() {
        let invalid = Variable::String("foo".to_string());
        let l = Variable::Number(Number::from_f64(10.0).unwrap());
        let r = Variable::Number(Number::from_f64(20.0).unwrap());
        assert_eq!(None, invalid.compare(&Comparator::GreaterThan, &r));
        assert_eq!(Some(false), l.compare(&Comparator::GreaterThan, &r));
        assert_eq!(Some(false), l.compare(&Comparator::GreaterThanEqual, &r));
        assert_eq!(Some(true), r.compare(&Comparator::GreaterThan, &l));
        assert_eq!(Some(true), r.compare(&Comparator::GreaterThanEqual, &l));
        assert_eq!(Some(true), l.compare(&Comparator::LessThan, &r));
        assert_eq!(Some(true), l.compare(&Comparator::LessThanEqual, &r));
        assert_eq!(Some(false), r.compare(&Comparator::LessThan, &l));
        assert_eq!(Some(false), r.compare(&Comparator::LessThanEqual, &l));
    }

    #[test]
    fn gets_value_from_object() {
        let var = Variable::from_json("{\"foo\":1}").unwrap();
        assert_eq!(
            Rcvar::new(Variable::Number(Number::from_f64(1.0).unwrap())),
            var.get_field("foo")
        );
    }

    #[test]
    fn getting_value_from_non_object_is_null() {
        assert_eq!(
            Rcvar::new(Variable::Null),
            Variable::Bool(false).get_field("foo")
        );
    }

    #[test]
    fn determines_if_null() {
        assert_eq!(false, Variable::Bool(true).is_null());
        assert_eq!(true, Variable::Null.is_null());
    }

    #[test]
    fn option_of_null() {
        assert_eq!(Some(()), Variable::Null.as_null());
        assert_eq!(None, Variable::Bool(true).as_null());
    }

    #[test]
    fn determines_if_boolean() {
        assert_eq!(true, Variable::Bool(true).is_boolean());
        assert_eq!(false, Variable::Null.is_boolean());
    }

    #[test]
    fn option_of_boolean() {
        assert_eq!(Some(true), Variable::Bool(true).as_boolean());
        assert_eq!(None, Variable::Null.as_boolean());
    }

    #[test]
    fn determines_if_string() {
        assert_eq!(false, Variable::Bool(true).is_string());
        assert_eq!(true, Variable::String("foo".to_string()).is_string());
    }

    #[test]
    fn option_of_string() {
        assert_eq!(
            Some(&"foo".to_string()),
            Variable::String("foo".to_string()).as_string()
        );
        assert_eq!(None, Variable::Null.as_string());
    }

    #[test]
    fn test_is_number() {
        let value = Variable::from_json("12").unwrap();
        assert!(value.is_number());
    }

    #[test]
    fn test_as_number() {
        let value = Variable::from_json("12.0").unwrap();
        assert_eq!(value.as_number(), Some(12f64));
    }

    #[test]
    fn test_is_object() {
        let value = Variable::from_json("{}").unwrap();
        assert!(value.is_object());
    }

    #[test]
    fn test_as_object() {
        let value = Variable::from_json("{}").unwrap();
        assert!(value.as_object().is_some());
    }

    #[test]
    fn test_is_array() {
        let value = Variable::from_json("[1, 2, 3]").unwrap();
        assert!(value.is_array());
    }

    #[test]
    fn test_as_array() {
        let value = Variable::from_json("[1, 2, 3]").unwrap();
        let array = value.as_array();
        let expected_length = 3;
        assert!(array.is_some() && array.unwrap().len() == expected_length);
    }

    #[test]
    fn test_is_expref() {
        assert_eq!(
            true,
            Variable::Expref(Ast::Identity { offset: 0 }).is_expref()
        );
        assert_eq!(
            &Ast::Identity { offset: 0 },
            Variable::Expref(Ast::Identity { offset: 0 })
                .as_expref()
                .unwrap()
        );
    }

    #[test]
    fn test_parses_json_scalar() {
        assert_eq!(Variable::Null, Variable::from_json("null").unwrap());
        assert_eq!(Variable::Bool(true), Variable::from_json("true").unwrap());
        assert_eq!(Variable::Bool(false), Variable::from_json("false").unwrap());
        assert_eq!(
            Variable::Number(Number::from(1)),
            Variable::from_json("1").unwrap()
        );
        assert_eq!(
            Variable::Number(Number::from_f64(-1.0).unwrap()),
            Variable::from_json("-1").unwrap()
        );
        assert_eq!(
            Variable::Number(Number::from_f64(1.5).unwrap()),
            Variable::from_json("1.5").unwrap()
        );
        assert_eq!(
            Variable::String("abc".to_string()),
            Variable::from_json("\"abc\"").unwrap()
        );
    }

    #[test]
    fn test_parses_and_encodes_complex() {
        let js = "[null,true,1,[\"a\"],{\"b\":{\"c\":[[9.9],false]}},-1,1.0000001]";
        let var = Variable::from_json(js).unwrap();
        assert_eq!(js, var.to_string());
    }

    #[test]
    fn test_parses_json_object() {
        let var = Variable::from_json("{\"a\": 1, \"b\": {\"c\": true}}").unwrap();
        let mut expected = BTreeMap::new();
        let mut sub_obj = BTreeMap::new();
        expected.insert(
            "a".to_string(),
            Rcvar::new(Variable::Number(Number::from_f64(1.0).unwrap())),
        );
        sub_obj.insert("c".to_string(), Rcvar::new(Variable::Bool(true)));
        expected.insert("b".to_string(), Rcvar::new(Variable::Object(sub_obj)));
        assert_eq!(var, Variable::Object(expected));
    }

    #[test]
    fn test_converts_to_json() {
        let var = Variable::from_json("true").unwrap();
        let val = serde_json::to_value(&var).unwrap();
        assert_eq!(Value::Bool(true), val);
        let round_trip = serde_json::from_value::<Variable>(val).unwrap();
        assert_eq!(Variable::Bool(true), round_trip);
        // Try a more complex shape
        let var = Variable::from_json("[\"a\"]").unwrap();
        let val = serde_json::to_value(&var).unwrap();
        assert_eq!(Value::Array(vec![Value::String("a".to_string())]), val);
        let round_trip = serde_json::from_value::<Variable>(val).unwrap();
        assert_eq!(
            Variable::Array(vec![Rcvar::new(Variable::String("a".to_string()))]),
            round_trip
        );
    }

    /// Converting an expression variable to a string is a special case.
    #[test]
    fn test_converts_to_string() {
        assert_eq!("true", Variable::Bool(true).to_string());
        assert_eq!("[true]", Variable::from_json("[true]").unwrap().to_string());
        let v = Variable::Expref(Ast::Identity { offset: 0 });
        assert_eq!("\"<expression: Identity { offset: 0 }>\"", v.to_string());
    }

    #[test]
    fn test_compares_float_equality() {
        assert_eq!(
            Variable::Number(Number::from_f64(1.0).unwrap()),
            Variable::Number(Number::from_f64(1.0).unwrap())
        );
        assert_eq!(
            Variable::Number(Number::from_f64(0.0).unwrap()),
            Variable::Number(Number::from_f64(0.0).unwrap())
        );
        assert_ne!(
            Variable::Number(Number::from_f64(0.00001).unwrap()),
            Variable::Number(Number::from_f64(0.0).unwrap())
        );
        assert_eq!(
            Variable::Number(Number::from_f64(999.999).unwrap()),
            Variable::Number(Number::from_f64(999.999).unwrap())
        );
        assert_eq!(
            Variable::Number(Number::from_f64(1.000000000001).unwrap()),
            Variable::Number(Number::from_f64(1.000000000001).unwrap())
        );
        assert_eq!(
            Variable::Number(Number::from_f64(0.7100000000000002).unwrap()),
            Variable::Number(Number::from_f64(0.71).unwrap())
        );
        assert_eq!(
            Variable::Number(Number::from_f64(0.0000000000000002).unwrap()),
            Variable::Number(Number::from_f64(0.0000000000000002).unwrap())
        );
        assert_ne!(
            Variable::Number(Number::from_f64(0.0000000000000002).unwrap()),
            Variable::Number(Number::from_f64(0.0000000000000003).unwrap())
        );
    }

    #[test]
    fn test_serialize_variable_with_positive_integer_value() {
        #[derive(serde_derive::Serialize)]
        struct Map {
            num: usize,
        }

        let map = Map { num: 231 };

        let var = Variable::from_serializable(&map).unwrap();
        let json_string = serde_json::to_string(&var).unwrap();

        assert_eq!(r#"{"num":231}"#, json_string);
    }

    #[test]
    fn test_serialize_variable_with_negative_integer_value() {
        #[derive(serde_derive::Serialize)]
        struct Map {
            num: isize,
        }

        let map = Map { num: -2141 };

        let var = Variable::from_serializable(&map).unwrap();
        let json_string = serde_json::to_string(&var).unwrap();

        assert_eq!(r#"{"num":-2141}"#, json_string);
    }

    #[test]
    fn test_serialize_variable_with_float_value() {
        #[derive(serde_derive::Serialize)]
        struct Map {
            num: f64,
        }

        let map = Map { num: 41.0 };

        let var = Variable::from_serializable(&map).unwrap();
        let json_string = serde_json::to_string(&var).unwrap();

        assert_eq!(r#"{"num":41.0}"#, json_string);
    }
}
