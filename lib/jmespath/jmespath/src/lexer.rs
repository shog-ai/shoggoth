//! Module for tokenizing JMESPath expressions.
//!
//! The lexer returns a VecDeque of tuples where each tuple contains the
//! character position in the original string from which the lexeme originates
//! followed by the token itself. The VecDeque is then consumed by the parser.
//! A VecDeque is utilized in order to pop owned tokens and provide arbitrary
//! token lookahead in the parser.

use std::collections::VecDeque;
use std::iter::Peekable;
use std::str::CharIndices;

use self::Token::*;
use crate::variable::Variable;
use crate::{ErrorReason, JmespathError, Rcvar};

/// Represents a lexical token of a JMESPath expression.
#[derive(Clone, PartialEq, Debug)]
pub enum Token {
    Identifier(String),
    QuotedIdentifier(String),
    Number(i32),
    Literal(Rcvar),
    Dot,
    Star,
    Flatten,
    And,
    Or,
    Pipe,
    Filter,
    Lbracket,
    Rbracket,
    Comma,
    Colon,
    Not,
    Ne,
    Eq,
    Gt,
    Gte,
    Lt,
    Lte,
    At,
    Ampersand,
    Lparen,
    Rparen,
    Lbrace,
    Rbrace,
    Eof,
}

impl Token {
    /// Provides the left binding power of the token.
    ///
    /// This is used in the parser to determine whether or not
    /// the currently parsing expression should continue parsing
    /// by consuming a token.
    #[inline]
    pub fn lbp(&self) -> usize {
        match *self {
            Pipe => 1,
            Or => 2,
            And => 3,
            Eq => 5,
            Gt => 5,
            Lt => 5,
            Gte => 5,
            Lte => 5,
            Ne => 5,
            Flatten => 9,
            Star => 20,
            Filter => 21,
            Dot => 40,
            Not => 45,
            Lbrace => 50,
            Lbracket => 55,
            Lparen => 60,
            _ => 0,
        }
    }
}

/// A tuple of the token position and the token.
pub type TokenTuple = (usize, Token);

/// Tokenizes a JMESPath expression.
pub fn tokenize(expr: &str) -> Result<VecDeque<TokenTuple>, JmespathError> {
    Lexer::new(expr).tokenize()
}

struct Lexer<'a> {
    iter: Peekable<CharIndices<'a>>,
    expr: &'a str,
}

impl<'a> Lexer<'a> {
    fn new(expr: &'a str) -> Lexer<'a> {
        Lexer {
            iter: expr.char_indices().peekable(),
            expr,
        }
    }

    fn tokenize(&mut self) -> Result<VecDeque<TokenTuple>, JmespathError> {
        let mut tokens = VecDeque::new();
        let last_position = self.expr.len();
        loop {
            match self.iter.next() {
                Some((pos, ch)) => {
                    match ch {
                        'a'..='z' | 'A'..='Z' | '_' => {
                            tokens.push_back((pos, self.consume_identifier(ch)))
                        }
                        '.' => tokens.push_back((pos, Dot)),
                        '[' => tokens.push_back((pos, self.consume_lbracket())),
                        '*' => tokens.push_back((pos, Star)),
                        '|' => tokens.push_back((pos, self.alt('|', Or, Pipe))),
                        '@' => tokens.push_back((pos, At)),
                        ']' => tokens.push_back((pos, Rbracket)),
                        '{' => tokens.push_back((pos, Lbrace)),
                        '}' => tokens.push_back((pos, Rbrace)),
                        '&' => tokens.push_back((pos, self.alt('&', And, Ampersand))),
                        '(' => tokens.push_back((pos, Lparen)),
                        ')' => tokens.push_back((pos, Rparen)),
                        ',' => tokens.push_back((pos, Comma)),
                        ':' => tokens.push_back((pos, Colon)),
                        '"' => tokens.push_back((pos, self.consume_quoted_identifier(pos)?)),
                        '\'' => tokens.push_back((pos, self.consume_raw_string(pos)?)),
                        '`' => tokens.push_back((pos, self.consume_literal(pos)?)),
                        '=' => match self.iter.next() {
                            Some((_, c)) if c == '=' => tokens.push_back((pos, Eq)),
                            _ => {
                                let message = "'=' is not valid. Did you mean '=='?";
                                let reason = ErrorReason::Parse(message.to_owned());
                                return Err(JmespathError::new(self.expr, pos, reason));
                            }
                        },
                        '>' => tokens.push_back((pos, self.alt('=', Gte, Gt))),
                        '<' => tokens.push_back((pos, self.alt('=', Lte, Lt))),
                        '!' => tokens.push_back((pos, self.alt('=', Ne, Not))),
                        '0'..='9' => tokens.push_back((pos, self.consume_number(ch, false))),
                        '-' => tokens.push_back((pos, self.consume_negative_number(pos)?)),
                        // Skip whitespace tokens
                        ' ' | '\n' | '\t' | '\r' => {}
                        c => {
                            let reason = ErrorReason::Parse(format!("Invalid character: {}", c));
                            return Err(JmespathError::new(self.expr, pos, reason));
                        }
                    }
                }
                None => {
                    tokens.push_back((last_position, Eof));
                    return Ok(tokens);
                }
            }
        }
    }

    // Consumes characters while the predicate function returns true.
    #[inline]
    fn consume_while<F>(&mut self, mut buffer: String, predicate: F) -> String
    where
        F: Fn(char) -> bool,
    {
        loop {
            match self.iter.peek() {
                None => break,
                Some(&(_, c)) if !predicate(c) => break,
                Some(&(_, c)) => {
                    buffer.push(c);
                    self.iter.next();
                }
            }
        }
        buffer
    }

    // Consumes "[", "[]", "[?
    #[inline]
    fn consume_lbracket(&mut self) -> Token {
        match self.iter.peek() {
            Some(&(_, ']')) => {
                self.iter.next();
                Flatten
            }
            Some(&(_, '?')) => {
                self.iter.next();
                Filter
            }
            _ => Lbracket,
        }
    }

    // Consume identifiers: ( ALPHA / "_" ) *( DIGIT / ALPHA / "_" )
    #[inline]
    fn consume_identifier(&mut self, first_char: char) -> Token {
        Identifier(self.consume_while(
            first_char.to_string(),
            |c| matches!(c, 'a'..='z' | '_' | 'A'..='Z' | '0'..='9'),
        ))
    }

    // Consumes numbers: *"-" "0" / ( %x31-39 *DIGIT )
    #[inline]
    fn consume_number(&mut self, first_char: char, is_negative: bool) -> Token {
        let lexeme = self.consume_while(first_char.to_string(), |c| c.is_digit(10));
        let numeric_value: i32 = lexeme.parse().expect("Expected valid number");
        if is_negative {
            Number(-numeric_value)
        } else {
            Number(numeric_value)
        }
    }

    // Consumes a negative number
    #[inline]
    fn consume_negative_number(&mut self, pos: usize) -> Result<Token, JmespathError> {
        // Ensure that the next value is a number > 0
        match self.iter.next() {
            Some((_, c)) if c.is_numeric() && c != '0' => Ok(self.consume_number(c, true)),
            _ => {
                let reason = ErrorReason::Parse("'-' must be followed by numbers 1-9".to_owned());
                Err(JmespathError::new(self.expr, pos, reason))
            }
        }
    }

    // Consumes tokens inside of a closing character. The closing character
    // can be escaped using a "\" character.
    #[inline]
    fn consume_inside<F>(
        &mut self,
        pos: usize,
        wrapper: char,
        invoke: F,
    ) -> Result<Token, JmespathError>
    where
        F: Fn(String) -> Result<Token, String>,
    {
        let mut buffer = String::new();
        while let Some((_, c)) = self.iter.next() {
            if c == wrapper {
                return invoke(buffer)
                    .map_err(|e| JmespathError::new(self.expr, pos, ErrorReason::Parse(e)));
            } else if c == '\\' {
                buffer.push(c);
                if let Some((_, c)) = self.iter.next() {
                    buffer.push(c);
                }
            } else {
                buffer.push(c)
            }
        }
        // The token was not closed, so error with the string, including the
        // wrapper (e.g., '"foo').
        let message = format!("Unclosed {} delimiter: {}{}", wrapper, wrapper, buffer);
        Err(JmespathError::new(
            self.expr,
            pos,
            ErrorReason::Parse(message),
        ))
    }

    // Consume and parse a quoted identifier token.
    #[inline]
    fn consume_quoted_identifier(&mut self, pos: usize) -> Result<Token, JmespathError> {
        self.consume_inside(pos, '"', |s| {
            // JSON decode the string to expand escapes
            match Variable::from_json(format!(r##""{}""##, s).as_ref()) {
                // Convert the JSON value into a string literal.
                Ok(j) => Ok(QuotedIdentifier(j.as_string().cloned().ok_or_else(
                    || "consume_quoted_identifier expected a string".to_owned(),
                )?)),
                Err(e) => Err(format!("Unable to parse quoted identifier {}: {}", s, e)),
            }
        })
    }

    #[inline]
    fn consume_raw_string(&mut self, pos: usize) -> Result<Token, JmespathError> {
        // Note: we need to unescape here because the backslashes are passed through.
        self.consume_inside(pos, '\'', |s| {
            Ok(Literal(Rcvar::new(Variable::String(s.replace("\\'", "'")))))
        })
    }

    // Consume and parse a literal JSON token.
    #[inline]
    fn consume_literal(&mut self, pos: usize) -> Result<Token, JmespathError> {
        self.consume_inside(pos, '`', |s| {
            let unescaped = s.replace("\\`", "`");
            match Variable::from_json(unescaped.as_ref()) {
                Ok(j) => Ok(Literal(Rcvar::new(j))),
                Err(err) => Err(format!("Unable to parse literal JSON {}: {}", s, err)),
            }
        })
    }

    #[inline]
    fn alt(&mut self, expected: char, match_type: Token, else_type: Token) -> Token {
        match self.iter.peek() {
            Some(&(_, c)) if c == expected => {
                self.iter.next();
                match_type
            }
            _ => else_type,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::variable::Variable;
    use crate::Rcvar;

    fn tokenize_queue(expr: &str) -> Vec<TokenTuple> {
        let mut result = tokenize(expr).unwrap();
        let mut v = Vec::new();
        while let Some(node) = result.pop_front() {
            v.push(node);
        }
        v
    }

    #[test]
    fn tokenize_basic_test() {
        assert_eq!(tokenize_queue("."), vec![(0, Dot), (1, Eof)]);
        assert_eq!(tokenize_queue("*"), vec![(0, Star), (1, Eof)]);
        assert_eq!(tokenize_queue("@"), vec![(0, At), (1, Eof)]);
        assert_eq!(tokenize_queue("]"), vec![(0, Rbracket), (1, Eof)]);
        assert_eq!(tokenize_queue("{"), vec![(0, Lbrace), (1, Eof)]);
        assert_eq!(tokenize_queue("}"), vec![(0, Rbrace), (1, Eof)]);
        assert_eq!(tokenize_queue("("), vec![(0, Lparen), (1, Eof)]);
        assert_eq!(tokenize_queue(")"), vec![(0, Rparen), (1, Eof)]);
        assert_eq!(tokenize_queue(","), vec![(0, Comma), (1, Eof)]);
    }

    #[test]
    fn tokenize_lbracket_test() {
        assert_eq!(tokenize_queue("["), vec![(0, Lbracket), (1, Eof)]);
        assert_eq!(tokenize_queue("[]"), vec![(0, Flatten), (2, Eof)]);
        assert_eq!(tokenize_queue("[?"), vec![(0, Filter), (2, Eof)]);
    }

    #[test]
    fn tokenize_pipe_test() {
        assert_eq!(tokenize_queue("|"), vec![(0, Pipe), (1, Eof)]);
        assert_eq!(tokenize_queue("||"), vec![(0, Or), (2, Eof)]);
    }

    #[test]
    fn tokenize_and_ampersand_test() {
        assert_eq!(tokenize_queue("&"), vec![(0, Ampersand), (1, Eof)]);
        assert_eq!(tokenize_queue("&&"), vec![(0, And), (2, Eof)]);
    }

    #[test]
    fn tokenize_lt_gt_test() {
        assert_eq!(tokenize_queue("<"), vec![(0, Lt), (1, Eof)]);
        assert_eq!(tokenize_queue("<="), vec![(0, Lte), (2, Eof)]);
        assert_eq!(tokenize_queue(">"), vec![(0, Gt), (1, Eof)]);
        assert_eq!(tokenize_queue(">="), vec![(0, Gte), (2, Eof)]);
    }

    #[test]
    fn tokenize_eq_ne_test() {
        assert_eq!(tokenize_queue("=="), vec![(0, Eq), (2, Eof)]);
        assert_eq!(tokenize_queue("!"), vec![(0, Not), (1, Eof)]);
        assert_eq!(tokenize_queue("!="), vec![(0, Ne), (2, Eof)]);
    }

    #[test]
    fn ensures_eq_valid() {
        assert!(tokenize("=").is_err());
    }

    #[test]
    fn skips_whitespace() {
        let tokens = tokenize_queue(" \t\n\r\t. (");
        assert_eq!(tokens, vec![(5, Dot), (7, Lparen), (8, Eof)]);
    }

    #[test]
    fn tokenize_single_error_test() {
        assert!(tokenize("~")
            .unwrap_err()
            .to_string()
            .contains("Invalid character: ~"));
    }

    #[test]
    fn tokenize_unclosed_errors_test() {
        assert!(tokenize("\"foo")
            .unwrap_err()
            .to_string()
            .contains("Unclosed \" delimiter: \"foo"));
        assert!(tokenize("`foo")
            .unwrap_err()
            .to_string()
            .contains("Unclosed ` delimiter: `foo"));
    }

    #[test]
    fn tokenize_identifier_test() {
        assert_eq!(
            tokenize_queue("foo_bar"),
            vec![(0, Identifier("foo_bar".to_string())), (7, Eof)]
        );
        assert_eq!(
            tokenize_queue("a"),
            vec![(0, Identifier("a".to_string())), (1, Eof)]
        );
        assert_eq!(
            tokenize_queue("_a"),
            vec![(0, Identifier("_a".to_string())), (2, Eof)]
        );
    }

    #[test]
    fn tokenize_quoted_identifier_test() {
        assert_eq!(
            tokenize_queue("\"foo\""),
            vec![(0, QuotedIdentifier("foo".to_string())), (5, Eof)]
        );
        assert_eq!(
            tokenize_queue("\"\""),
            vec![(0, QuotedIdentifier("".to_string())), (2, Eof)]
        );
        assert_eq!(
            tokenize_queue("\"a_b\""),
            vec![(0, QuotedIdentifier("a_b".to_string())), (5, Eof)]
        );
        assert_eq!(
            tokenize_queue("\"a\\nb\""),
            vec![(0, QuotedIdentifier("a\nb".to_string())), (6, Eof)]
        );
        assert_eq!(
            tokenize_queue("\"a\\\\nb\""),
            vec![(0, QuotedIdentifier("a\\nb".to_string())), (7, Eof)]
        );
    }

    #[test]
    fn tokenize_raw_string_test() {
        assert_eq!(
            tokenize_queue("'foo'"),
            vec![
                (0, Literal(Rcvar::new(Variable::String("foo".to_string())))),
                (5, Eof)
            ]
        );
        assert_eq!(
            tokenize_queue("''"),
            vec![
                (0, Literal(Rcvar::new(Variable::String("".to_string())))),
                (2, Eof)
            ]
        );
        assert_eq!(
            tokenize_queue("'a\\nb'"),
            vec![
                (
                    0,
                    Literal(Rcvar::new(Variable::String("a\\nb".to_string())))
                ),
                (6, Eof)
            ]
        );
    }

    #[test]
    fn tokenize_literal_test() {
        // Must enclose in quotes. See JEP 12.
        assert!(tokenize("`a`")
            .unwrap_err()
            .to_string()
            .contains("Unable to parse"));
        assert_eq!(
            tokenize_queue("`\"a\"`"),
            vec![
                (0, Literal(Rcvar::new(Variable::String("a".to_string())))),
                (5, Eof)
            ]
        );
        assert_eq!(
            tokenize_queue("`\"a b\"`"),
            vec![
                (0, Literal(Rcvar::new(Variable::String("a b".to_string())))),
                (7, Eof)
            ]
        );
    }

    #[test]
    fn tokenize_number_test() {
        assert_eq!(tokenize_queue("0"), vec![(0, Number(0)), (1, Eof)]);
        assert_eq!(tokenize_queue("1"), vec![(0, Number(1)), (1, Eof)]);
        assert_eq!(tokenize_queue("123"), vec![(0, Number(123)), (3, Eof)]);
    }

    #[test]
    fn tokenize_negative_number_test() {
        assert_eq!(tokenize_queue("-10"), vec![(0, Number(-10)), (3, Eof)]);
    }

    #[test]
    fn tokenize_negative_number_test_failure() {
        assert!(tokenize("-01").unwrap_err().to_string().contains("'-'"));
    }

    #[test]
    fn tokenize_successive_test() {
        let expr = "foo.bar || `\"a\"` | 10";
        let tokens = tokenize_queue(expr);
        assert_eq!(tokens[0], (0, Identifier("foo".to_string())));
        assert_eq!(tokens[1], (3, Dot));
        assert_eq!(tokens[2], (4, Identifier("bar".to_string())));
        assert_eq!(tokens[3], (8, Or));
        assert_eq!(
            tokens[4],
            (11, Literal(Rcvar::new(Variable::String("a".to_string()))))
        );
        assert_eq!(tokens[5], (17, Pipe));
        assert_eq!(tokens[6], (19, Number(10)));
        assert_eq!(tokens[7], (21, Eof));
    }

    #[test]
    fn tokenizes_slices() {
        let tokens = tokenize_queue("foo[0::-1]");
        assert_eq!(
            "[(0, Identifier(\"foo\")), (3, Lbracket), (4, Number(0)), (5, Colon), \
                     (6, Colon), (7, Number(-1)), (9, Rbracket), (10, Eof)]",
            format!("{:?}", tokens)
        );
    }
}
