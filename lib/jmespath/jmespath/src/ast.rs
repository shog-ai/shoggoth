//! JMESPath abstract syntax tree (AST).
//!
//! Inspecting the JMESPath AST can be useful for analyzing the way in
//! which an expression was parsed and which features are utilized in
//! an expression.
//!
//! Ast can be accessed directly from a parsed `jmespath::Expression`
//! using the `as_ast()` method. An Ast can be created by using the
//! `jmespath::parse()` function which returns an Ast rather than an
//! `Expression`.
//!
//! ```
//! use jmespath;
//!
//! let ast = jmespath::parse("a || b && c").unwrap();
//! ```

use std::fmt;

use crate::lexer::Token;
use crate::Rcvar;

/// A JMESPath expression abstract syntax tree.
#[derive(Clone, PartialEq, Debug)]
pub enum Ast {
    /// Compares two nodes using a comparator, returning true/false.
    Comparison {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Comparator that compares the two results
        comparator: Comparator,
        /// Left hand side of the comparison
        lhs: Box<Ast>,
        /// Right hand side of the comparison
        rhs: Box<Ast>,
    },
    /// If `predicate` evaluates to a truthy value, returns the
    /// result `then`
    Condition {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// The predicate to test.
        predicate: Box<Ast>,
        /// The node to traverse if the predicate is truthy.
        then: Box<Ast>,
    },
    /// Returns the current node.
    Identity {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
    },
    /// Used by functions to dynamically evaluate argument values.
    Expref {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Node to execute
        ast: Box<Ast>,
    },
    /// Evaluates the node, then flattens it one level.
    Flatten {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Node to execute and flatten
        node: Box<Ast>,
    },
    /// Function name and a vec or function argument expressions.
    Function {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Function name to invoke.
        name: String,
        /// Function arguments.
        args: Vec<Ast>,
    },
    /// Extracts a key by name from a map.
    Field {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Field name to extract.
        name: String,
    },
    /// Extracts an index from a Vec.
    Index {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Index to extract
        idx: i32,
    },
    /// Resolves to a literal value.
    Literal {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Literal value
        value: Rcvar,
    },
    /// Evaluates to a list of evaluated expressions.
    MultiList {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Elements of the list
        elements: Vec<Ast>,
    },
    /// Evaluates to a map of key value pairs.
    MultiHash {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Elements of the hash
        elements: Vec<KeyValuePair>,
    },
    /// Evaluates to true/false based on if the expression is not truthy.
    Not {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// node to negate
        node: Box<Ast>,
    },
    /// Evaluates LHS, and pushes each value through RHS.
    Projection {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Left hand side of the projection.
        lhs: Box<Ast>,
        /// Right hand side of the projection.
        rhs: Box<Ast>,
    },
    /// Evaluates LHS. If it resolves to an object, returns a Vec of values.
    ObjectValues {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Node to extract object values from.
        node: Box<Ast>,
    },
    /// Evaluates LHS. If not truthy returns. Otherwise evaluates RHS.
    And {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Left hand side of the expression.
        lhs: Box<Ast>,
        /// Right hand side of the expression.
        rhs: Box<Ast>,
    },
    /// Evaluates LHS. If truthy returns. Otherwise evaluates RHS.
    Or {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Left hand side of the expression.
        lhs: Box<Ast>,
        /// Right hand side of the expression.
        rhs: Box<Ast>,
    },
    /// Returns a slice of a vec, using start, stop, and step.
    Slice {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Starting index
        start: Option<i32>,
        /// Stopping index
        stop: Option<i32>,
        /// Step amount between extractions.
        step: i32,
    },
    /// Evaluates RHS, then provides that value to the evaluation of RHS.
    Subexpr {
        /// Approximate absolute position in the parsed expression.
        offset: usize,
        /// Left hand side of the expression.
        lhs: Box<Ast>,
        /// Right hand side of the expression.
        rhs: Box<Ast>,
    },
}

impl fmt::Display for Ast {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        write!(fmt, "{:#?}", self)
    }
}

/// Represents a key value pair in a MultiHash.
#[derive(Clone, PartialEq, Debug)]
pub struct KeyValuePair {
    /// Key name.
    pub key: String,
    /// Value expression used to determine the value.
    pub value: Ast,
}

/// Comparators used in Comparison nodes.
#[derive(Clone, PartialEq, Debug)]
pub enum Comparator {
    Equal,
    NotEqual,
    LessThan,
    LessThanEqual,
    GreaterThan,
    GreaterThanEqual,
}

/// Creates a Comparator from a Token.
///
/// Note: panics if the Token is invalid.
impl From<Token> for Comparator {
    fn from(token: Token) -> Self {
        match token {
            Token::Lt => Comparator::LessThan,
            Token::Lte => Comparator::LessThanEqual,
            Token::Gt => Comparator::GreaterThan,
            Token::Gte => Comparator::GreaterThanEqual,
            Token::Eq => Comparator::Equal,
            Token::Ne => Comparator::NotEqual,
            _ => panic!("Invalid token for comparator: {:?}", token),
        }
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn displays_pretty_printed_ast_node() {
        let node = Ast::Field {
            name: "abc".to_string(),
            offset: 4,
        };
        assert_eq!(
            "Field {\n    offset: 4,\n    name: \"abc\",\n}",
            format!("{}", node)
        );
    }
}
