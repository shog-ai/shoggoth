use std::collections::HashMap;

use crate::functions::*;
use crate::parse;
use crate::Expression;
use crate::JmespathError;

/// Compiles JMESPath expressions.
///
/// Most use cases don't need to worry about how Runtime works.
/// You really only need to create your own Runtimes if you are
/// utilizing custom functions in your expressions.
pub struct Runtime {
    functions: HashMap<String, Box<dyn Function>>,
}

impl Default for Runtime {
    fn default() -> Self {
        Runtime {
            functions: HashMap::with_capacity(26),
        }
    }
}

impl Runtime {
    /// Creates a new Runtime.
    pub fn new() -> Runtime {
        Default::default()
    }

    /// Creates a new JMESPath expression from an expression string.
    ///
    /// The provided expression is expected to adhere to the JMESPath
    /// grammar: <https://jmespath.org/specification.html>
    #[inline]
    pub fn compile<'a>(&'a self, expression: &str) -> Result<Expression<'a>, JmespathError> {
        parse(expression).map(|ast| Expression::new(expression, ast, self))
    }

    /// Adds a new function to the runtime.
    #[inline]
    pub fn register_function(&mut self, name: &str, f: Box<dyn Function>) {
        self.functions.insert(name.to_owned(), f);
    }

    /// Removes a function from the runtime.
    ///
    /// Returns the function that was removed if it was found.
    pub fn deregister_function(&mut self, name: &str) -> Option<Box<dyn Function>> {
        self.functions.remove(name)
    }

    /// Gets a function by name from the runtime.
    #[inline]
    pub fn get_function<'a>(&'a self, name: &str) -> Option<&'a dyn Function> {
        self.functions.get(name).map(AsRef::as_ref)
    }

    /// Registers all of the builtin JMESPath functions with the runtime.
    pub fn register_builtin_functions(&mut self) {
        self.register_function("abs", Box::new(AbsFn::new()));
        self.register_function("avg", Box::new(AvgFn::new()));
        self.register_function("ceil", Box::new(CeilFn::new()));
        self.register_function("contains", Box::new(ContainsFn::new()));
        self.register_function("ends_with", Box::new(EndsWithFn::new()));
        self.register_function("floor", Box::new(FloorFn::new()));
        self.register_function("join", Box::new(JoinFn::new()));
        self.register_function("keys", Box::new(KeysFn::new()));
        self.register_function("length", Box::new(LengthFn::new()));
        self.register_function("map", Box::new(MapFn::new()));
        self.register_function("min", Box::new(MinFn::new()));
        self.register_function("max", Box::new(MaxFn::new()));
        self.register_function("max_by", Box::new(MaxByFn::new()));
        self.register_function("min_by", Box::new(MinByFn::new()));
        self.register_function("merge", Box::new(MergeFn::new()));
        self.register_function("not_null", Box::new(NotNullFn::new()));
        self.register_function("reverse", Box::new(ReverseFn::new()));
        self.register_function("sort", Box::new(SortFn::new()));
        self.register_function("sort_by", Box::new(SortByFn::new()));
        self.register_function("starts_with", Box::new(StartsWithFn::new()));
        self.register_function("sum", Box::new(SumFn::new()));
        self.register_function("to_array", Box::new(ToArrayFn::new()));
        self.register_function("to_number", Box::new(ToNumberFn::new()));
        self.register_function("to_string", Box::new(ToStringFn::new()));
        self.register_function("type", Box::new(TypeFn::new()));
        self.register_function("values", Box::new(ValuesFn::new()));
    }
}
