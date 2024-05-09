//! Module for parsing JMESPath expressions into an AST.
//!
//! This JMESPath parser is implemented using a Pratt parser,
//! or top down operator precedence parser:
//! <https://tdop.github.io/>

use std::collections::VecDeque;

use crate::ast::{Ast, Comparator, KeyValuePair};
use crate::lexer::{tokenize, Token, TokenTuple};
use crate::{ErrorReason, JmespathError};

/// Result of parsing an expression.
pub type ParseResult = Result<Ast, JmespathError>;

/// Parses a JMESPath expression into an AST.
pub fn parse(expr: &str) -> ParseResult {
    let tokens = tokenize(expr)?;
    Parser::new(tokens, expr).parse()
}

/// The maximum binding power for a token that can stop a projection.
const PROJECTION_STOP: usize = 10;

struct Parser<'a> {
    /// Parsed tokens
    token_queue: VecDeque<TokenTuple>,
    /// Shared EOF token
    eof_token: Token,
    /// Expression being parsed
    expr: &'a str,
    /// The current character offset in the expression
    offset: usize,
}

impl<'a> Parser<'a> {
    fn new(tokens: VecDeque<TokenTuple>, expr: &'a str) -> Parser<'a> {
        Parser {
            token_queue: tokens,
            eof_token: Token::Eof,
            offset: 0,
            expr,
        }
    }

    #[inline]
    fn parse(&mut self) -> ParseResult {
        self.expr(0).and_then(|result| {
            // After parsing the expr, we should reach the end of the stream.
            match self.peek(0) {
                &Token::Eof => Ok(result),
                t => Err(self.err(t, "Did not parse the complete expression", true)),
            }
        })
    }

    #[inline]
    fn advance(&mut self) -> Token {
        self.advance_with_pos().1
    }

    #[inline]
    fn advance_with_pos(&mut self) -> (usize, Token) {
        match self.token_queue.pop_front() {
            Some((pos, tok)) => {
                self.offset = pos;
                (pos, tok)
            }
            None => (self.offset, Token::Eof),
        }
    }

    #[inline]
    fn peek(&self, lookahead: usize) -> &Token {
        match self.token_queue.get(lookahead) {
            Some(&(_, ref t)) => t,
            None => &self.eof_token,
        }
    }

    /// Returns a formatted error with the given message.
    fn err(&self, current_token: &Token, error_msg: &str, is_peek: bool) -> JmespathError {
        let mut actual_pos = self.offset;
        let mut buff = error_msg.to_string();
        buff.push_str(&format!(" -- found {:?}", current_token));
        if is_peek {
            if let Some(&(p, _)) = self.token_queue.get(0) {
                actual_pos = p;
            }
        }
        JmespathError::new(self.expr, actual_pos, ErrorReason::Parse(buff))
    }

    /// Main parse function of the Pratt parser that parses while RBP < LBP
    fn expr(&mut self, rbp: usize) -> ParseResult {
        let mut left = self.nud();
        while rbp < self.peek(0).lbp() {
            left = self.led(Box::new(left?));
        }
        left
    }

    fn nud(&mut self) -> ParseResult {
        let (offset, token) = self.advance_with_pos();
        match token {
            Token::At => Ok(Ast::Identity { offset }),
            Token::Identifier(value) => Ok(Ast::Field {
                name: value,
                offset,
            }),
            Token::QuotedIdentifier(value) => match self.peek(0) {
                Token::Lparen => {
                    let message = "Quoted strings can't be a function name";
                    Err(self.err(&Token::Lparen, message, true))
                }
                _ => Ok(Ast::Field {
                    name: value,
                    offset,
                }),
            },
            Token::Star => self.parse_wildcard_values(Box::new(Ast::Identity { offset })),
            Token::Literal(value) => Ok(Ast::Literal { value, offset }),
            Token::Lbracket => match self.peek(0) {
                &Token::Number(_) | &Token::Colon => self.parse_index(),
                &Token::Star if self.peek(1) == &Token::Rbracket => {
                    self.advance();
                    self.parse_wildcard_index(Box::new(Ast::Identity { offset }))
                }
                _ => self.parse_multi_list(),
            },
            Token::Flatten => self.parse_flatten(Box::new(Ast::Identity { offset })),
            Token::Lbrace => {
                let mut pairs = vec![];
                loop {
                    // Requires at least on key value pair.
                    pairs.push(self.parse_kvp()?);
                    match self.advance() {
                        // Terminal condition is the Rbrace token
                        Token::Rbrace => break,
                        // Skip commas as they are used to delineate kvps
                        Token::Comma => continue,
                        ref t => return Err(self.err(t, "Expected '}' or ','", false)),
                    }
                }
                Ok(Ast::MultiHash {
                    elements: pairs,
                    offset,
                })
            }
            t @ Token::Ampersand => {
                let rhs = self.expr(t.lbp())?;
                Ok(Ast::Expref {
                    ast: Box::new(rhs),
                    offset,
                })
            }
            t @ Token::Not => Ok(Ast::Not {
                node: Box::new(self.expr(t.lbp())?),
                offset,
            }),
            Token::Filter => self.parse_filter(Box::new(Ast::Identity { offset })),
            Token::Lparen => {
                let result = self.expr(0)?;
                match self.advance() {
                    Token::Rparen => Ok(result),
                    ref t => Err(self.err(t, "Expected ')' to close '('", false)),
                }
            }
            ref t => Err(self.err(t, "Unexpected nud token", false)),
        }
    }

    fn led(&mut self, left: Box<Ast>) -> ParseResult {
        let (offset, token) = self.advance_with_pos();
        match token {
            t @ Token::Dot => {
                if self.peek(0) == &Token::Star {
                    // Skip the star and parse the rhs
                    self.advance();
                    self.parse_wildcard_values(left)
                } else {
                    let offset = offset;
                    let rhs = self.parse_dot(t.lbp())?;
                    Ok(Ast::Subexpr {
                        offset,
                        lhs: left,
                        rhs: Box::new(rhs),
                    })
                }
            }
            Token::Lbracket => {
                if match self.peek(0) {
                    &Token::Number(_) | &Token::Colon => true,
                    &Token::Star => false,
                    t => return Err(self.err(t, "Expected number, ':', or '*'", true)),
                } {
                    Ok(Ast::Subexpr {
                        offset,
                        lhs: left,
                        rhs: Box::new(self.parse_index()?),
                    })
                } else {
                    self.advance();
                    self.parse_wildcard_index(left)
                }
            }
            t @ Token::Or => {
                let offset = offset;
                let rhs = self.expr(t.lbp())?;
                Ok(Ast::Or {
                    offset,
                    lhs: left,
                    rhs: Box::new(rhs),
                })
            }
            t @ Token::And => {
                let offset = offset;
                let rhs = self.expr(t.lbp())?;
                Ok(Ast::And {
                    offset,
                    lhs: left,
                    rhs: Box::new(rhs),
                })
            }
            t @ Token::Pipe => {
                let offset = offset;
                let rhs = self.expr(t.lbp())?;
                Ok(Ast::Subexpr {
                    offset,
                    lhs: left,
                    rhs: Box::new(rhs),
                })
            }
            Token::Lparen => match *left {
                Ast::Field { name: v, .. } => Ok(Ast::Function {
                    offset,
                    name: v,
                    args: self.parse_list(Token::Rparen)?,
                }),
                _ => Err(self.err(self.peek(0), "Invalid function name", true)),
            },
            Token::Flatten => self.parse_flatten(left),
            Token::Filter => self.parse_filter(left),
            Token::Eq => self.parse_comparator(Comparator::Equal, left),
            Token::Ne => self.parse_comparator(Comparator::NotEqual, left),
            Token::Gt => self.parse_comparator(Comparator::GreaterThan, left),
            Token::Gte => self.parse_comparator(Comparator::GreaterThanEqual, left),
            Token::Lt => self.parse_comparator(Comparator::LessThan, left),
            Token::Lte => self.parse_comparator(Comparator::LessThanEqual, left),
            ref t => Err(self.err(t, "Unexpected led token", false)),
        }
    }

    fn parse_kvp(&mut self) -> Result<KeyValuePair, JmespathError> {
        match self.advance() {
            Token::Identifier(value) | Token::QuotedIdentifier(value) => {
                if self.peek(0) == &Token::Colon {
                    self.advance();
                    Ok(KeyValuePair {
                        key: value,
                        value: self.expr(0)?,
                    })
                } else {
                    Err(self.err(self.peek(0), "Expected ':' to follow key", true))
                }
            }
            ref t => Err(self.err(t, "Expected Field to start key value pair", false)),
        }
    }

    /// Parses a filter token into a Projection that filters the right
    /// side of the projection using a Condition node. If the Condition node
    /// returns a truthy value, then the value is yielded by the projection.
    fn parse_filter(&mut self, lhs: Box<Ast>) -> ParseResult {
        // Parse the LHS of the condition node.
        let condition_lhs = Box::new(self.expr(0)?);
        // Eat the closing bracket.
        match self.advance() {
            Token::Rbracket => {
                let condition_rhs = Box::new(self.projection_rhs(Token::Filter.lbp())?);
                Ok(Ast::Projection {
                    offset: self.offset,
                    lhs,
                    rhs: Box::new(Ast::Condition {
                        offset: self.offset,
                        predicate: condition_lhs,
                        then: condition_rhs,
                    }),
                })
            }
            ref t => Err(self.err(t, "Expected ']'", false)),
        }
    }

    fn parse_flatten(&mut self, lhs: Box<Ast>) -> ParseResult {
        let rhs = Box::new(self.projection_rhs(Token::Flatten.lbp())?);
        Ok(Ast::Projection {
            offset: self.offset,
            lhs: Box::new(Ast::Flatten {
                offset: self.offset,
                node: lhs,
            }),
            rhs,
        })
    }

    /// Parses a comparator token into a Comparison (e.g., foo == bar)
    fn parse_comparator(&mut self, cmp: Comparator, lhs: Box<Ast>) -> ParseResult {
        let rhs = Box::new(self.expr(Token::Eq.lbp())?);
        Ok(Ast::Comparison {
            offset: self.offset,
            comparator: cmp,
            lhs,
            rhs,
        })
    }

    /// Parses the right hand side of a dot expression.
    fn parse_dot(&mut self, lbp: usize) -> ParseResult {
        if match self.peek(0) {
            &Token::Lbracket => true,
            &Token::Identifier(_)
            | &Token::QuotedIdentifier(_)
            | &Token::Star
            | &Token::Lbrace
            | &Token::Ampersand => false,
            t => return Err(self.err(t, "Expected identifier, '*', '{', '[', '&', or '[?'", true)),
        } {
            self.advance();
            self.parse_multi_list()
        } else {
            self.expr(lbp)
        }
    }

    /// Parses the right hand side of a projection, using the given LBP to
    /// determine when to stop consuming tokens.
    fn projection_rhs(&mut self, lbp: usize) -> ParseResult {
        if match self.peek(0) {
            &Token::Dot => true,
            &Token::Lbracket | &Token::Filter => false,
            t if t.lbp() < PROJECTION_STOP => {
                return Ok(Ast::Identity {
                    offset: self.offset,
                });
            }
            t => {
                return Err(self.err(t, "Expected '.', '[', or '[?'", true));
            }
        } {
            self.advance();
            self.parse_dot(lbp)
        } else {
            self.expr(lbp)
        }
    }

    /// Creates a projection for "[*]"
    fn parse_wildcard_index(&mut self, lhs: Box<Ast>) -> ParseResult {
        match self.advance() {
            Token::Rbracket => {
                let rhs = Box::new(self.projection_rhs(Token::Star.lbp())?);
                Ok(Ast::Projection {
                    offset: self.offset,
                    lhs,
                    rhs,
                })
            }
            ref t => Err(self.err(t, "Expected ']' for wildcard index", false)),
        }
    }

    /// Creates a projection for "*"
    fn parse_wildcard_values(&mut self, lhs: Box<Ast>) -> ParseResult {
        let rhs = Box::new(self.projection_rhs(Token::Star.lbp())?);
        Ok(Ast::Projection {
            offset: self.offset,
            lhs: Box::new(Ast::ObjectValues {
                offset: self.offset,
                node: lhs,
            }),
            rhs,
        })
    }

    /// Parses [0], [::-1], [0:-1], [0:1], etc...
    fn parse_index(&mut self) -> ParseResult {
        let mut parts = [None, None, None];
        let mut pos = 0;
        loop {
            match self.advance() {
                Token::Number(value) => {
                    parts[pos] = Some(value);
                    match self.peek(0) {
                        &Token::Colon | &Token::Rbracket => (),
                        t => return Err(self.err(t, "Expected ':', or ']'", true)),
                    };
                }
                Token::Rbracket => break,
                Token::Colon if pos >= 2 => {
                    return Err(self.err(&Token::Colon, "Too many colons in slice expr", false));
                }
                Token::Colon => {
                    pos += 1;
                    match self.peek(0) {
                        &Token::Number(_) | &Token::Colon | &Token::Rbracket => continue,
                        t => return Err(self.err(t, "Expected number, ':', or ']'", true)),
                    };
                }
                ref t => return Err(self.err(t, "Expected number, ':', or ']'", false)),
            }
        }

        if pos == 0 {
            // No colons were found, so this is a simple index extraction.
            Ok(Ast::Index {
                offset: self.offset,
                idx: parts[0].ok_or_else(|| {
                    JmespathError::new(
                        self.expr,
                        self.offset,
                        ErrorReason::Parse(
                            "Expected parts[0] to be Some; but found None".to_owned(),
                        ),
                    )
                })?,
            })
        } else {
            // Sliced array from start (e.g., [2:])
            Ok(Ast::Projection {
                offset: self.offset,
                lhs: Box::new(Ast::Slice {
                    offset: self.offset,
                    start: parts[0],
                    stop: parts[1],
                    step: parts[2].unwrap_or(1),
                }),
                rhs: Box::new(self.projection_rhs(Token::Star.lbp())?),
            })
        }
    }

    /// Parses multi-select lists (e.g., "[foo, bar, baz]")
    fn parse_multi_list(&mut self) -> ParseResult {
        Ok(Ast::MultiList {
            offset: self.offset,
            elements: self.parse_list(Token::Rbracket)?,
        })
    }

    /// Parse a comma separated list of expressions until a closing token.
    ///
    /// This function is used for functions and multi-list parsing. Note
    /// that this function allows empty lists. This is fine when parsing
    /// multi-list expressions because "[]" is tokenized as Token::Flatten.
    ///
    /// Examples: [foo, bar], foo(bar), foo(), foo(baz, bar)
    fn parse_list(&mut self, closing: Token) -> Result<Vec<Ast>, JmespathError> {
        let mut nodes = vec![];
        while self.peek(0) != &closing {
            nodes.push(self.expr(0)?);
            // Skip commas
            if self.peek(0) == &Token::Comma {
                self.advance();
                if self.peek(0) == &closing {
                    return Err(self.err(self.peek(0), "invalid token after ','", true));
                }
            }
        }
        self.advance();
        Ok(nodes)
    }
}
