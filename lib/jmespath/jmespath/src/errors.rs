//! JMESPath errors.

use std::error::Error;
use std::fmt;

use crate::Context;

/// JMESPath error.
#[derive(Clone, Debug, PartialEq)]
pub struct JmespathError {
    /// Absolute character position.
    pub offset: usize,
    /// Line number of the coordinate.
    pub line: usize,
    /// Column of the line number.
    pub column: usize,
    /// Expression being evaluated.
    pub expression: String,
    /// Error reason information.
    pub reason: ErrorReason,
}

impl JmespathError {
    /// Create a new JMESPath Error.
    pub fn new(expr: &str, offset: usize, reason: ErrorReason) -> JmespathError {
        // Find each new line so we can create a formatted error message.
        let mut line: usize = 0;
        let mut column: usize = 0;
        for c in expr.chars().take(offset) {
            match c {
                '\n' => {
                    line += 1;
                    column = 0;
                }
                _ => column += 1,
            }
        }
        JmespathError {
            expression: expr.to_owned(),
            offset,
            line,
            column,
            reason,
        }
    }

    /// Create a new JMESPath Error from a Context struct.
    pub fn from_ctx(ctx: &Context<'_>, reason: ErrorReason) -> JmespathError {
        JmespathError::new(ctx.expression, ctx.offset, reason)
    }
}

impl Error for JmespathError {
    fn description(&self) -> &str {
        "error evaluating JMESPath expression"
    }
}

impl From<serde_json::Error> for JmespathError {
    fn from(err: serde_json::Error) -> Self {
        JmespathError::new(
            "",
            0,
            ErrorReason::Parse(format!("Serde parse error: {}", err)),
        )
    }
}

fn inject_carat(column: usize, buff: &mut String) {
    buff.push_str(&(0..column).map(|_| ' ').collect::<String>());
    buff.push_str("^\n");
}

impl fmt::Display for JmespathError {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        let mut error_location = String::new();
        let mut matched = false;
        let mut current_line = 0;
        for c in self.expression.chars() {
            error_location.push(c);
            if c == '\n' {
                current_line += 1;
                if current_line == self.line + 1 {
                    matched = true;
                    inject_carat(self.column, &mut error_location);
                }
            }
        }
        if !matched {
            error_location.push('\n');
            inject_carat(self.column, &mut error_location);
        }

        write!(
            fmt,
            "{} (line {}, column {})\n{}",
            self.reason, self.line, self.column, error_location
        )
    }
}

/// Error context to provide specific details about an error.
#[derive(Clone, Debug, PartialEq)]
pub enum ErrorReason {
    /// An error occurred while parsing an expression.
    Parse(String),
    /// An error occurred while evaluating an expression.
    Runtime(RuntimeError),
}

impl fmt::Display for ErrorReason {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        match *self {
            ErrorReason::Parse(ref e) => write!(fmt, "Parse error: {}", e),
            ErrorReason::Runtime(ref e) => write!(fmt, "Runtime error: {}", e),
        }
    }
}

/// Runtime JMESPath error
#[derive(Clone, Debug, PartialEq)]
pub enum RuntimeError {
    /// Encountered when a slice expression uses a step of 0
    InvalidSlice,
    /// Encountered when too many arguments are provided to a function.
    TooManyArguments {
        /// Expeced number of arguments.
        expected: usize,
        /// Provided number of arguments.
        actual: usize,
    },
    /// Encountered when too few arguments are provided to a function.
    NotEnoughArguments {
        /// Expeced number of arguments.
        expected: usize,
        /// Provided number of arguments.
        actual: usize,
    },
    /// Encountered when an unknown function is called.
    UnknownFunction(String),
    /// Encountered when a type of variable given to a function is invalid.
    InvalidType {
        /// Expected type.
        expected: String,
        /// Provided type.
        actual: String,
        /// Argument position when calling the function.
        position: usize,
    },
    /// Encountered when an expression reference returns an invalid type.
    InvalidReturnType {
        /// Expected return type.
        expected: String,
        /// Actual return type.
        actual: String,
        /// Argument position from which the expression reference was invoked.
        position: usize,
        /// Which invocation iteration of the expression reference failed.
        invocation: usize,
    },
}

impl fmt::Display for RuntimeError {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        use self::RuntimeError::*;
        match *self {
            UnknownFunction(ref function) => write!(fmt, "Call to undefined function {}", function),
            TooManyArguments {
                ref expected,
                ref actual,
            } => write!(
                fmt,
                "Too many arguments: expected {}, found {}",
                expected, actual
            ),
            NotEnoughArguments {
                ref expected,
                ref actual,
            } => write!(
                fmt,
                "Not enough arguments: expected {}, found {}",
                expected, actual
            ),
            InvalidType {
                ref expected,
                ref actual,
                ref position,
            } => write!(
                fmt,
                "Argument {} expects type {}, given {}",
                position, expected, actual
            ),
            InvalidSlice => write!(fmt, "Invalid slice"),
            InvalidReturnType {
                ref expected,
                ref actual,
                ref position,
                ref invocation,
            } => write!(
                fmt,
                "Argument {} must return {} but invocation {} returned {}",
                position, expected, invocation, actual
            ),
        }
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn coordinates_can_be_created_from_string_with_new_lines() {
        let expr = "foo\n..bar";
        let err = JmespathError::new(&expr, 5, ErrorReason::Parse("Test".to_owned()));
        assert_eq!(1, err.line);
        assert_eq!(1, err.column);
        assert_eq!(5, err.offset);
        assert_eq!(
            "Parse error: Test (line 1, column 1)\nfoo\n..bar\n ^\n",
            err.to_string()
        );
    }

    #[test]
    fn coordinates_can_be_created_from_string_with_new_lines_pointing_to_non_last() {
        let expr = "foo\n..bar\nbaz";
        let err = JmespathError::new(&expr, 5, ErrorReason::Parse("Test".to_owned()));
        assert_eq!(1, err.line);
        assert_eq!(1, err.column);
        assert_eq!(5, err.offset);
        assert_eq!(
            "Parse error: Test (line 1, column 1)\nfoo\n..bar\n ^\nbaz",
            err.to_string()
        );
    }

    #[test]
    fn coordinates_can_be_created_from_string_with_no_new_lines() {
        let expr = "foo..bar";
        let err = JmespathError::new(&expr, 4, ErrorReason::Parse("Test".to_owned()));
        assert_eq!(0, err.line);
        assert_eq!(4, err.column);
        assert_eq!(4, err.offset);
        assert_eq!(
            "Parse error: Test (line 0, column 4)\nfoo..bar\n    ^\n",
            err.to_string()
        );
    }

    #[test]
    fn reason_displays_parse_errors() {
        let reason = ErrorReason::Parse("bar".to_owned());
        assert_eq!("Parse error: bar", reason.to_string());
    }

    #[test]
    fn reason_displays_runtime_errors() {
        let reason = ErrorReason::Runtime(RuntimeError::UnknownFunction("a".to_owned()));
        assert_eq!(
            "Runtime error: Call to undefined function a",
            reason.to_string()
        );
    }

    #[test]
    fn displays_invalid_type_error() {
        let error = RuntimeError::InvalidType {
            expected: "string".to_owned(),
            actual: "boolean".to_owned(),
            position: 0,
        };
        assert_eq!(
            "Argument 0 expects type string, given boolean",
            error.to_string()
        );
    }

    #[test]
    fn displays_invalid_slice() {
        let error = RuntimeError::InvalidSlice;
        assert_eq!("Invalid slice", error.to_string());
    }

    #[test]
    fn displays_too_many_arguments_error() {
        let error = RuntimeError::TooManyArguments {
            expected: 1,
            actual: 2,
        };
        assert_eq!("Too many arguments: expected 1, found 2", error.to_string());
    }

    #[test]
    fn displays_not_enough_arguments_error() {
        let error = RuntimeError::NotEnoughArguments {
            expected: 2,
            actual: 1,
        };
        assert_eq!(
            "Not enough arguments: expected 2, found 1",
            error.to_string()
        );
    }

    #[test]
    fn displays_invalid_return_type_error() {
        let error = RuntimeError::InvalidReturnType {
            expected: "string".to_string(),
            actual: "boolean".to_string(),
            position: 0,
            invocation: 2,
        };
        assert_eq!(
            "Argument 0 must return string but invocation 2 returned boolean",
            error.to_string()
        );
    }
}
