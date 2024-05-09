//! JMESPath functions.

use std::cmp::{max, min};
use std::collections::BTreeMap;
use std::fmt;

use crate::interpreter::{interpret, SearchResult};
use crate::variable::{JmespathType, Variable};
use crate::{Context, ErrorReason, JmespathError, Rcvar, RuntimeError};
use serde_json::Number;

/// Represents a JMESPath function.
pub trait Function: Sync {
    /// Evaluates the function against an in-memory variable.
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult;
}

/// Function argument types used when validating.
#[derive(Clone, PartialEq, Eq, Debug)]
pub enum ArgumentType {
    Any,
    Null,
    String,
    Number,
    Bool,
    Object,
    Array,
    Expref,
    /// Each element of the array must matched the provided type.
    TypedArray(Box<ArgumentType>),
    /// Accepts one of a number of `ArgumentType`s
    Union(Vec<ArgumentType>),
}

impl ArgumentType {
    /// Returns true/false if the variable is valid for the type.
    pub fn is_valid(&self, value: &Rcvar) -> bool {
        use self::ArgumentType::*;
        match *self {
            Any => true,
            Null if value.is_null() => true,
            String if value.is_string() => true,
            Number if value.is_number() => true,
            Object if value.is_object() => true,
            Bool if value.is_boolean() => true,
            Expref if value.is_expref() => true,
            Array if value.is_array() => true,
            TypedArray(ref t) if value.is_array() => {
                if let Some(array) = value.as_array() {
                    array.iter().all(|v| t.is_valid(v))
                } else {
                    false
                }
            }
            Union(ref types) => types.iter().any(|t| t.is_valid(value)),
            _ => false,
        }
    }
}

impl fmt::Display for ArgumentType {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        use self::ArgumentType::*;
        match *self {
            Any => write!(fmt, "any"),
            String => write!(fmt, "string"),
            Number => write!(fmt, "number"),
            Bool => write!(fmt, "boolean"),
            Array => write!(fmt, "array"),
            Object => write!(fmt, "object"),
            Null => write!(fmt, "null"),
            Expref => write!(fmt, "expref"),
            TypedArray(ref t) => write!(fmt, "array[{}]", t),
            Union(ref types) => {
                let str_value = types
                    .iter()
                    .map(|t| t.to_string())
                    .collect::<Vec<_>>()
                    .join("|");
                write!(fmt, "{}", str_value)
            }
        }
    }
}

macro_rules! arg {
    (any) => (ArgumentType::Any);
    (null) => (ArgumentType::Null);
    (string) => (ArgumentType::String);
    (bool) => (ArgumentType::Bool);
    (number) => (ArgumentType::Number);
    (object) => (ArgumentType::Object);
    (expref) => (ArgumentType::Expref);
    (array_number) => (ArgumentType::TypedArray(Box::new(ArgumentType::Number)));
    (array_string) => (ArgumentType::TypedArray(Box::new(ArgumentType::String)));
    (array) => (ArgumentType::Array);
    ($($x:ident) | *) => (ArgumentType::Union(vec![$(arg!($x)), *]));
}

/// Custom function that allows the creation of runtime functions with signature validation.
pub struct CustomFunction {
    /// Signature used to validate the function.
    signature: Signature,
    /// Function to invoke after validating the signature.
    f: Box<dyn Fn(&[Rcvar], &mut Context<'_>) -> SearchResult + Sync>,
}

impl CustomFunction {
    /// Creates a new custom function.
    pub fn new(
        fn_signature: Signature,
        f: Box<dyn Fn(&[Rcvar], &mut Context<'_>) -> SearchResult + Sync>,
    ) -> CustomFunction {
        CustomFunction {
            signature: fn_signature,
            f,
        }
    }
}

impl Function for CustomFunction {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        (self.f)(args, ctx)
    }
}

/// Normal closures can be used as functions.
///
/// It is up to the function to validate the provided arguments.
/// If you wish to utilize Signatures or more complex argument
/// validation, it is recommended to use CustomFunction.
impl<F> Function for F
where
    F: Sync + Fn(&[Rcvar], &mut Context<'_>) -> SearchResult,
{
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        (self)(args, ctx)
    }
}

/// Represents a function's signature.
#[derive(Clone, PartialEq, Eq, Debug)]
pub struct Signature {
    pub inputs: Vec<ArgumentType>,
    pub variadic: Option<ArgumentType>,
}

impl Signature {
    /// Creates a new Signature struct.
    pub fn new(inputs: Vec<ArgumentType>, variadic: Option<ArgumentType>) -> Signature {
        Signature { inputs, variadic }
    }

    /// Validates the arity of a function. If the arity is invalid, a runtime
    /// error is returned with the relative position of the error and the
    /// expression that was being executed.
    pub fn validate_arity(&self, actual: usize, ctx: &Context<'_>) -> Result<(), JmespathError> {
        let expected = self.inputs.len();
        if self.variadic.is_some() {
            if actual >= expected {
                Ok(())
            } else {
                let reason =
                    ErrorReason::Runtime(RuntimeError::NotEnoughArguments { expected, actual });
                Err(JmespathError::from_ctx(ctx, reason))
            }
        } else if actual == expected {
            Ok(())
        } else if actual < expected {
            let reason =
                ErrorReason::Runtime(RuntimeError::NotEnoughArguments { expected, actual });
            Err(JmespathError::from_ctx(ctx, reason))
        } else {
            let reason = ErrorReason::Runtime(RuntimeError::TooManyArguments { expected, actual });
            Err(JmespathError::from_ctx(ctx, reason))
        }
    }

    /// Validates the provided function arguments against the signature.
    pub fn validate(&self, args: &[Rcvar], ctx: &Context<'_>) -> Result<(), JmespathError> {
        self.validate_arity(args.len(), ctx)?;
        if let Some(ref variadic) = self.variadic {
            for (k, v) in args.iter().enumerate() {
                let validator = self.inputs.get(k).unwrap_or(variadic);
                self.validate_arg(ctx, k, v, validator)?;
            }
        } else {
            for (k, v) in args.iter().enumerate() {
                self.validate_arg(ctx, k, v, &self.inputs[k])?;
            }
        }
        Ok(())
    }

    fn validate_arg(
        &self,
        ctx: &Context<'_>,
        position: usize,
        value: &Rcvar,
        validator: &ArgumentType,
    ) -> Result<(), JmespathError> {
        if validator.is_valid(value) {
            Ok(())
        } else {
            let reason = ErrorReason::Runtime(RuntimeError::InvalidType {
                expected: validator.to_string(),
                actual: value.get_type().to_string(),
                position,
            });
            Err(JmespathError::from_ctx(ctx, reason))
        }
    }
}

/// Macro to more easily and quickly define a function and signature.
macro_rules! defn {
    ($name:ident, $args:expr, $variadic:expr) => {
        pub struct $name {
            signature: Signature,
        }

        impl Default for $name {
            fn default() -> Self {
                Self::new()
            }
        }

        impl $name {
            pub fn new() -> $name {
                $name {
                    signature: Signature::new($args, $variadic),
                }
            }
        }
    };
}

/// Macro used to implement max_by and min_by functions.
macro_rules! min_and_max_by {
    ($ctx:expr, $operator:ident, $args:expr) => {{
        let vals = $args[0].as_array().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be an array".to_owned()),
            )
        })?;
        // Return null when there are not values in the array
        if vals.is_empty() {
            return Ok(Rcvar::new(Variable::Null));
        }
        let ast = $args[1].as_expref().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[1] to be an expression".to_owned()),
            )
        })?;
        // Map over the first value to get the homogeneous required return type
        let initial = interpret(&vals[0], &ast, $ctx)?;
        let entered_type = initial.get_type();
        if entered_type != JmespathType::String && entered_type != JmespathType::Number {
            return Err(JmespathError::from_ctx(
                $ctx,
                ErrorReason::Runtime(RuntimeError::InvalidReturnType {
                    expected: "expression->number|expression->string".to_owned(),
                    actual: entered_type.to_string(),
                    position: 1,
                    invocation: 1,
                }),
            ));
        }
        // Map over each value, finding the best candidate value and fail on error.
        let mut candidate = (vals[0].clone(), initial.clone());
        for (invocation, v) in vals.iter().enumerate().skip(1) {
            let mapped = interpret(v, &ast, $ctx)?;
            if mapped.get_type() != entered_type {
                return Err(JmespathError::from_ctx(
                    $ctx,
                    ErrorReason::Runtime(RuntimeError::InvalidReturnType {
                        expected: format!("expression->{}", entered_type),
                        actual: mapped.get_type().to_string(),
                        position: 1,
                        invocation,
                    }),
                ));
            }
            if mapped.$operator(&candidate.1) {
                candidate = (v.clone(), mapped);
            }
        }
        Ok(candidate.0)
    }};
}

/// Macro used to implement max and min functions.
macro_rules! min_and_max {
    ($operator:ident, $args:expr) => {{
        let values = $args[0].as_array().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be an array".to_owned()),
            )
        })?;
        if values.is_empty() {
            Ok(Rcvar::new(Variable::Null))
        } else {
            let result: Rcvar = values
                .iter()
                .skip(1)
                .fold(values[0].clone(), |acc, item| $operator(acc, item.clone()));
            Ok(result)
        }
    }};
}

defn!(AbsFn, vec![arg!(number)], None);

impl Function for AbsFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        match args[0].as_ref() {
            Variable::Number(n) => Ok(Rcvar::new(Variable::Number(
                Number::from_f64(
                    n.as_f64()
                        .ok_or_else(|| {
                            JmespathError::new(
                                "",
                                0,
                                ErrorReason::Parse("Expected to be a valid f64".to_owned()),
                            )
                        })?
                        .abs(),
                )
                .ok_or_else(|| {
                    JmespathError::new(
                        "",
                        0,
                        ErrorReason::Parse("Expected to be a valid f64".to_owned()),
                    )
                })?,
            ))),
            _ => Ok(args[0].clone()),
        }
    }
}

defn!(AvgFn, vec![arg!(array_number)], None);

impl Function for AvgFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let values = args[0].as_array().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be an array".to_owned()),
            )
        })?;

        let mut sum = 0.0;

        for value in values {
            sum += value.as_number().ok_or_else(|| {
                JmespathError::new(
                    "",
                    0,
                    ErrorReason::Parse("Expected to be a valid f64".to_owned()),
                )
            })?;
        }

        Ok(Rcvar::new(Variable::Number(
            Number::from_f64(sum / (values.len() as f64)).ok_or_else(|| {
                JmespathError::new(
                    "",
                    0,
                    ErrorReason::Parse("Expected to be a valid f64".to_owned()),
                )
            })?,
        )))
    }
}

defn!(CeilFn, vec![arg!(number)], None);

impl Function for CeilFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let n = args[0].as_number().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be a number".to_owned()),
            )
        })?;
        Ok(Rcvar::new(Variable::Number(
            Number::from_f64(n.ceil()).ok_or_else(|| {
                JmespathError::new(
                    "",
                    0,
                    ErrorReason::Parse("Expected n.ceil() to be a valid f64".to_owned()),
                )
            })?,
        )))
    }
}

defn!(ContainsFn, vec![arg!(string | array), arg!(any)], None);

impl Function for ContainsFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let haystack = &args[0];
        let needle = &args[1];
        match **haystack {
            Variable::Array(ref a) => Ok(Rcvar::new(Variable::Bool(a.contains(needle)))),
            Variable::String(ref subj) => match needle.as_string() {
                None => Ok(Rcvar::new(Variable::Bool(false))),
                Some(s) => Ok(Rcvar::new(Variable::Bool(subj.contains(s)))),
            },
            _ => unreachable!(),
        }
    }
}

defn!(EndsWithFn, vec![arg!(string), arg!(string)], None);

impl Function for EndsWithFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let subject = args[0].as_string().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be a valid string".to_owned()),
            )
        })?;
        let search = args[1].as_string().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[1] to be a valid string".to_owned()),
            )
        })?;
        Ok(Rcvar::new(Variable::Bool(subject.ends_with(search))))
    }
}

defn!(FloorFn, vec![arg!(number)], None);

impl Function for FloorFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let n = args[0].as_number().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be a valid number".to_owned()),
            )
        })?;
        Ok(Rcvar::new(Variable::Number(
            Number::from_f64(n.floor()).ok_or_else(|| {
                JmespathError::new(
                    "",
                    0,
                    ErrorReason::Parse("Expected to be a valid number".to_owned()),
                )
            })?,
        )))
    }
}

defn!(JoinFn, vec![arg!(string), arg!(array_string)], None);

impl Function for JoinFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let glue = args[0].as_string().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be a valid string".to_owned()),
            )
        })?;
        let values = args[1].as_array().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[1] to be a valid string".to_owned()),
            )
        })?;
        let result = values
            .iter()
            .map(|v| {
                v.as_string().map(|val| val.to_owned()).ok_or_else(|| {
                    JmespathError::new(
                        "",
                        0,
                        ErrorReason::Parse("Expected to be a valid string".to_owned()),
                    )
                })
            })
            .collect::<Result<Vec<String>, JmespathError>>()?
            .join(glue);
        Ok(Rcvar::new(Variable::String(result)))
    }
}

defn!(KeysFn, vec![arg!(object)], None);

impl Function for KeysFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let object = args[0].as_object().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be a valid Object".to_owned()),
            )
        })?;
        let keys = object
            .keys()
            .map(|k| Rcvar::new(Variable::String((*k).clone())))
            .collect::<Vec<Rcvar>>();
        Ok(Rcvar::new(Variable::Array(keys)))
    }
}

defn!(LengthFn, vec![arg!(array | object | string)], None);

impl Function for LengthFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        match args[0].as_ref() {
            Variable::Array(ref a) => Ok(Rcvar::new(Variable::Number(Number::from(a.len())))),
            Variable::Object(ref m) => Ok(Rcvar::new(Variable::Number(Number::from(m.len())))),
            // Note that we need to count the code points not the number of unicode characters
            Variable::String(ref s) => Ok(Rcvar::new(Variable::Number(Number::from(
                s.chars().count(),
            )))),
            _ => unreachable!(),
        }
    }
}

defn!(MapFn, vec![arg!(expref), arg!(array)], None);

impl Function for MapFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let ast = args[0].as_expref().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be an expref".to_owned()),
            )
        })?;
        let values = args[1].as_array().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[1] to be an array".to_owned()),
            )
        })?;
        let mut results = vec![];
        for value in values {
            results.push(interpret(value, ast, ctx)?);
        }
        Ok(Rcvar::new(Variable::Array(results)))
    }
}

defn!(MaxFn, vec![arg!(array_string | array_number)], None);

impl Function for MaxFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        min_and_max!(max, args)
    }
}

defn!(MinFn, vec![arg!(array_string | array_number)], None);

impl Function for MinFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        min_and_max!(min, args)
    }
}

defn!(MaxByFn, vec![arg!(array), arg!(expref)], None);

impl Function for MaxByFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        min_and_max_by!(ctx, gt, args)
    }
}

defn!(MinByFn, vec![arg!(array), arg!(expref)], None);

impl Function for MinByFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        min_and_max_by!(ctx, lt, args)
    }
}

defn!(MergeFn, vec![arg!(object)], Some(arg!(object)));

impl Function for MergeFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let mut result = BTreeMap::new();
        for arg in args {
            result.extend(
                arg.as_object()
                    .ok_or_else(|| {
                        JmespathError::new(
                            "",
                            0,
                            ErrorReason::Parse("Expected to be a valid Object".to_owned()),
                        )
                    })?
                    .clone(),
            );
        }
        Ok(Rcvar::new(Variable::Object(result)))
    }
}

defn!(NotNullFn, vec![arg!(any)], Some(arg!(any)));

impl Function for NotNullFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        for arg in args {
            if !arg.is_null() {
                return Ok(arg.clone());
            }
        }
        Ok(Rcvar::new(Variable::Null))
    }
}

defn!(ReverseFn, vec![arg!(array | string)], None);

impl Function for ReverseFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        if args[0].is_array() {
            let mut values = args[0]
                .as_array()
                .ok_or_else(|| {
                    JmespathError::new(
                        "",
                        0,
                        ErrorReason::Parse("Expected args[0] to be an array".to_owned()),
                    )
                })?
                .clone();
            values.reverse();
            Ok(Rcvar::new(Variable::Array(values)))
        } else {
            let word: String = args[0]
                .as_string()
                .ok_or_else(|| {
                    JmespathError::new(
                        "",
                        0,
                        ErrorReason::Parse("Expected args[0] to be a string".to_owned()),
                    )
                })?
                .chars()
                .rev()
                .collect();
            Ok(Rcvar::new(Variable::String(word)))
        }
    }
}

defn!(SortFn, vec![arg!(array_string | array_number)], None);

impl Function for SortFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let mut values = args[0]
            .as_array()
            .ok_or_else(|| {
                JmespathError::new(
                    "",
                    0,
                    ErrorReason::Parse("Expected args[0] to be an array".to_owned()),
                )
            })?
            .clone();
        values.sort();
        Ok(Rcvar::new(Variable::Array(values)))
    }
}

defn!(SortByFn, vec![arg!(array), arg!(expref)], None);

impl Function for SortByFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let vals = args[0]
            .as_array()
            .ok_or_else(|| {
                JmespathError::new(
                    "",
                    0,
                    ErrorReason::Parse("Expected args[0] to be an array".to_owned()),
                )
            })?
            .clone();
        if vals.is_empty() {
            return Ok(Rcvar::new(Variable::Array(vals)));
        }
        let ast = args[1].as_expref().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[1] to be an expref".to_owned()),
            )
        })?;
        let mut mapped: Vec<(Rcvar, Rcvar)> = vec![];
        let first_value = interpret(&vals[0], ast, ctx)?;
        let first_type = first_value.get_type();
        if first_type != JmespathType::String && first_type != JmespathType::Number {
            let reason = ErrorReason::Runtime(RuntimeError::InvalidReturnType {
                expected: "expression->string|expression->number".to_owned(),
                actual: first_type.to_string(),
                position: 1,
                invocation: 1,
            });
            return Err(JmespathError::from_ctx(ctx, reason));
        }
        mapped.push((vals[0].clone(), first_value));
        for (invocation, v) in vals.iter().enumerate().skip(1) {
            let mapped_value = interpret(v, ast, ctx)?;
            if mapped_value.get_type() != first_type {
                return Err(JmespathError::from_ctx(
                    ctx,
                    ErrorReason::Runtime(RuntimeError::InvalidReturnType {
                        expected: format!("expression->{}", first_type),
                        actual: mapped_value.get_type().to_string(),
                        position: 1,
                        invocation,
                    }),
                ));
            }
            mapped.push((v.clone(), mapped_value));
        }
        mapped.sort_by(|a, b| a.1.cmp(&b.1));
        let result = mapped.iter().map(|tuple| tuple.0.clone()).collect();
        Ok(Rcvar::new(Variable::Array(result)))
    }
}

defn!(StartsWithFn, vec![arg!(string), arg!(string)], None);

impl Function for StartsWithFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let subject = args[0].as_string().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[0] to be a string".to_owned()),
            )
        })?;
        let search = args[1].as_string().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[1] to be a string".to_owned()),
            )
        })?;
        Ok(Rcvar::new(Variable::Bool(subject.starts_with(search))))
    }
}

defn!(SumFn, vec![arg!(array_number)], None);

impl Function for SumFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let result = args[0]
            .as_array()
            .ok_or_else(|| {
                JmespathError::new(
                    "",
                    0,
                    ErrorReason::Parse("Expected args[0] to be an array".to_owned()),
                )
            })?
            .iter()
            .fold(0.0, |acc, item| acc + item.as_number().unwrap_or(0.0));
        Ok(Rcvar::new(Variable::Number(
            Number::from_f64(result).ok_or_else(|| {
                JmespathError::new(
                    "",
                    0,
                    ErrorReason::Parse("Expected to be a valid number".to_owned()),
                )
            })?,
        )))
    }
}

defn!(ToArrayFn, vec![arg!(any)], None);

impl Function for ToArrayFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        match *args[0] {
            Variable::Array(_) => Ok(args[0].clone()),
            _ => Ok(Rcvar::new(Variable::Array(vec![args[0].clone()]))),
        }
    }
}

defn!(ToNumberFn, vec![arg!(any)], None);

impl Function for ToNumberFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        match *args[0] {
            Variable::Number(_) => Ok(args[0].clone()),
            Variable::String(ref s) => match Variable::from_json(s) {
                Ok(f) => Ok(Rcvar::new(f)),
                Err(_) => Ok(Rcvar::new(Variable::Null)),
            },
            _ => Ok(Rcvar::new(Variable::Null)),
        }
    }
}

defn!(
    ToStringFn,
    vec![arg!(object | array | bool | number | string | null)],
    None
);

impl Function for ToStringFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        match *args[0] {
            Variable::String(_) => Ok(args[0].clone()),
            _ => Ok(Rcvar::new(Variable::String(args[0].to_string()))),
        }
    }
}

defn!(TypeFn, vec![arg!(any)], None);

impl Function for TypeFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        Ok(Rcvar::new(Variable::String(args[0].get_type().to_string())))
    }
}

defn!(ValuesFn, vec![arg!(object)], None);

impl Function for ValuesFn {
    fn evaluate(&self, args: &[Rcvar], ctx: &mut Context<'_>) -> SearchResult {
        self.signature.validate(args, ctx)?;
        let map = args[0].as_object().ok_or_else(|| {
            JmespathError::new(
                "",
                0,
                ErrorReason::Parse("Expected args[1] to be an Object".to_owned()),
            )
        })?;
        Ok(Rcvar::new(Variable::Array(
            map.values().cloned().collect::<Vec<Rcvar>>(),
        )))
    }
}
