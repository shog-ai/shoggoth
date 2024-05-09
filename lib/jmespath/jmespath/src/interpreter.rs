//! Interprets JMESPath expressions.

use std::collections::BTreeMap;

use super::ast::Ast;
use super::variable::Variable;
use super::Context;
use super::{ErrorReason, JmespathError, Rcvar, RuntimeError};

/// Result of searching data using a JMESPath Expression.
pub type SearchResult = Result<Rcvar, JmespathError>;

/// Interprets the given data using an AST node.
pub fn interpret(data: &Rcvar, node: &Ast, ctx: &mut Context<'_>) -> SearchResult {
    match *node {
        Ast::Field { ref name, .. } => Ok(data.get_field(name)),
        Ast::Subexpr {
            ref lhs, ref rhs, ..
        } => {
            let left_result = interpret(data, lhs, ctx)?;
            interpret(&left_result, rhs, ctx)
        }
        Ast::Identity { .. } => Ok(data.clone()),
        Ast::Literal { ref value, .. } => Ok(value.clone()),
        Ast::Index { idx, .. } => {
            if idx >= 0 {
                Ok(data.get_index(idx as usize))
            } else {
                Ok(data.get_negative_index((-idx) as usize))
            }
        }
        Ast::Or {
            ref lhs, ref rhs, ..
        } => {
            let left = interpret(data, lhs, ctx)?;
            if left.is_truthy() {
                Ok(left)
            } else {
                interpret(data, rhs, ctx)
            }
        }
        Ast::And {
            ref lhs, ref rhs, ..
        } => {
            let left = interpret(data, lhs, ctx)?;
            if !left.is_truthy() {
                Ok(left)
            } else {
                interpret(data, rhs, ctx)
            }
        }
        Ast::Not { ref node, .. } => {
            let result = interpret(data, node, ctx)?;
            Ok(Rcvar::new(Variable::Bool(!result.is_truthy())))
        }
        // Returns the resut of RHS if cond yields truthy value.
        Ast::Condition {
            ref predicate,
            ref then,
            ..
        } => {
            let cond_result = interpret(data, predicate, ctx)?;
            if cond_result.is_truthy() {
                interpret(data, then, ctx)
            } else {
                Ok(Rcvar::new(Variable::Null))
            }
        }
        Ast::Comparison {
            ref comparator,
            ref lhs,
            ref rhs,
            ..
        } => {
            let left = interpret(data, lhs, ctx)?;
            let right = interpret(data, rhs, ctx)?;
            Ok(left
                .compare(comparator, &*right)
                .map_or(Rcvar::new(Variable::Null), |result| {
                    Rcvar::new(Variable::Bool(result))
                }))
        }
        // Converts an object into a JSON array of its values.
        Ast::ObjectValues { ref node, .. } => {
            let subject = interpret(data, node, ctx)?;
            match *subject {
                Variable::Object(ref v) => Ok(Rcvar::new(Variable::Array(
                    v.values().cloned().collect::<Vec<Rcvar>>(),
                ))),
                _ => Ok(Rcvar::new(Variable::Null)),
            }
        }
        // Passes the results of lhs into rhs if lhs yields an array and
        // each node of lhs that passes through rhs yields a non-null value.
        Ast::Projection {
            ref lhs, ref rhs, ..
        } => match interpret(data, lhs, ctx)?.as_array() {
            None => Ok(Rcvar::new(Variable::Null)),
            Some(left) => {
                let mut collected = vec![];
                for element in left {
                    let current = interpret(element, rhs, ctx)?;
                    if !current.is_null() {
                        collected.push(current);
                    }
                }
                Ok(Rcvar::new(Variable::Array(collected)))
            }
        },
        Ast::Flatten { ref node, .. } => match interpret(data, node, ctx)?.as_array() {
            None => Ok(Rcvar::new(Variable::Null)),
            Some(a) => {
                let mut collected: Vec<Rcvar> = vec![];
                for element in a {
                    match element.as_array() {
                        Some(array) => collected.extend(array.iter().cloned()),
                        _ => collected.push(element.clone()),
                    }
                }
                Ok(Rcvar::new(Variable::Array(collected)))
            }
        },
        Ast::MultiList { ref elements, .. } => {
            if data.is_null() {
                Ok(Rcvar::new(Variable::Null))
            } else {
                let mut collected = vec![];
                for node in elements {
                    collected.push(interpret(data, node, ctx)?);
                }
                Ok(Rcvar::new(Variable::Array(collected)))
            }
        }
        Ast::MultiHash { ref elements, .. } => {
            if data.is_null() {
                Ok(Rcvar::new(Variable::Null))
            } else {
                let mut collected = BTreeMap::new();
                for kvp in elements {
                    let value = interpret(data, &kvp.value, ctx)?;
                    collected.insert(kvp.key.clone(), value);
                }
                Ok(Rcvar::new(Variable::Object(collected)))
            }
        }
        Ast::Function {
            ref name,
            ref args,
            offset,
        } => {
            let mut fn_args: Vec<Rcvar> = vec![];
            for arg in args {
                fn_args.push(interpret(data, arg, ctx)?);
            }
            // Reset the offset so that it points to the function being evaluated.
            ctx.offset = offset;
            match ctx.runtime.get_function(name) {
                Some(f) => f.evaluate(&fn_args, ctx),
                None => {
                    let reason =
                        ErrorReason::Runtime(RuntimeError::UnknownFunction(name.to_owned()));
                    Err(JmespathError::from_ctx(ctx, reason))
                }
            }
        }
        Ast::Expref { ref ast, .. } => Ok(Rcvar::new(Variable::Expref(*ast.clone()))),
        Ast::Slice {
            start,
            stop,
            step,
            offset,
        } => {
            if step == 0 {
                ctx.offset = offset;
                let reason = ErrorReason::Runtime(RuntimeError::InvalidSlice);
                Err(JmespathError::from_ctx(ctx, reason))
            } else {
                match data.slice(start, stop, step) {
                    Some(array) => Ok(Rcvar::new(Variable::Array(array))),
                    None => Ok(Rcvar::new(Variable::Null)),
                }
            }
        }
    }
}
