//! Generated benchmarks. Benchmarks are generated in build.rs and are
//! sourced from tests/compliance/benchmarks.json
//#![feature(test)]

//extern crate bencher;

use bencher::*;
use jmespath::{compile, parse, Rcvar, Variable};

include!(concat!(env!("OUT_DIR"), "/benches.rs"));
