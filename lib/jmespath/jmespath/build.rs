//! Generates compliance tests and benchmarks

use std::env;
use std::fs::{self, File};
use std::io::{Read, Write};
use std::path::Path;

use serde_json::Value;

pub fn main() {
    let out_dir = env::var_os("OUT_DIR").expect("OUT_DIR not specified");
    let bench_path = Path::new(&out_dir).join("benches.rs");
    let mut bench_file = File::create(&bench_path).expect("Could not create file");
    let compliance_path = Path::new(&out_dir).join("compliance_tests.rs");
    let mut compliance_file = File::create(&compliance_path).expect("Could not create file");
    let suites = load_test_suites();

    let mut all_benches_test = vec![];

    for (suite_num, &(ref filename, ref suite)) in suites.iter().enumerate() {
        let suite_obj = suite.as_object().expect("Suite not object");
        let given = suite_obj.get("given").expect("No given value");
        let cases = suite_obj.get("cases").expect("No cases value");
        let short_filename = filename
            .replace(".json", "")
            .replace("tests/compliance/", "");
        let given_string = serde_json::to_string(given).unwrap();
        for (case_num, case) in cases
            .as_array()
            .expect("cases not array")
            .iter()
            .enumerate()
        {
            let case_obj = case.as_object().expect("case not object");
            if case_obj.get("bench").is_some() {
                generate_bench(
                    &mut all_benches_test,
                    &short_filename,
                    suite_num,
                    case_num,
                    case_obj,
                    &given_string,
                    &mut bench_file,
                );
            } else {
                generate_test(
                    &short_filename,
                    suite_num,
                    case_num,
                    case_obj,
                    &given_string,
                    &mut compliance_file,
                );
            }
        }
    }
    bench_file
        .write_all(
            format!(
                r#"
benchmark_group!(benches, {});
benchmark_main!(benches);
"#,
                all_benches_test.join(", ")
            )
            .as_bytes(),
        )
        .expect("Error bench headers");
}

/// Load all tests suites found in the tests/compliance directory.
pub fn load_test_suites() -> Vec<(String, Value)> {
    let mut result = vec![];
    let files = fs::read_dir("tests/compliance").expect("Invalid directory: tests/compliance");
    for filename in files {
        let path = filename.expect("Invalid file").path();
        let file_path = path.to_str().expect("Could not to_str file").to_string();
        let mut f = File::open(path).expect("Unable to open file");
        let mut file_data = String::new();
        f.read_to_string(&mut file_data)
            .expect("Could to read JSON to string");
        let mut suite_json: Value = serde_json::from_str(&file_data).expect("invalid JSON");
        let suites = suite_json
            .as_array_mut()
            .expect("Test suite is not a JSON array");
        while let Some(suite) = suites.pop() {
            result.push((file_path.clone(), suite));
        }
    }
    result
}

/// Gets the expression from a test case with helpful error messages.
#[inline]
fn get_expr(case: &serde_json::Map<String, Value>) -> &str {
    case.get("expression")
        .expect("No expression in case")
        .as_str()
        .expect("Could not convert case to string")
}

/// SLugifies a string for use as a function name.
/// Ensures that the generated slug is truncated if too long.
#[inline]
fn slugify(s: &str) -> String {
    let mut s = slug::slugify(s).replace("-", "_");
    if s.len() > 25 {
        s.truncate(25);
    }
    s
}

/// Generates a function name for a test suite's test case.
#[inline]
fn generate_fn_name(
    filename: &str,
    suite_num: usize,
    case_num: usize,
    case: &serde_json::Map<String, Value>,
) -> String {
    let expr = get_expr(case);
    // Use the comment as the fn description if it is present.
    let description = match case.get("comment") {
        Some(c) => c.as_str().expect("comment is not a string"),
        None => expr,
    };
    format!(
        "{}_{}_{}_{}",
        slugify(filename),
        suite_num,
        case_num,
        slugify(description)
    )
}

/// Generates a benchmark to be run with cargo bench.
///
/// Each test case will generate a case for parsing and a case for both
/// parsing and interpreting.
fn generate_bench(
    all_tests: &mut Vec<String>,
    filename: &str,
    suite_num: usize,
    case_num: usize,
    case: &serde_json::Map<String, Value>,
    given_string: &str,
    f: &mut File,
) {
    let expr = get_expr(case);
    let expr_string = expr.replace("\"", "\\\"");
    let fn_suffix = generate_fn_name(filename, suite_num, case_num, case);
    let bench_type = case
        .get("bench")
        .unwrap()
        .as_str()
        .expect("bench is not a string");

    // Validate that the bench attribute is an expected type.
    if bench_type != "parse" && bench_type != "full" && bench_type != "interpret" {
        panic!("invalid bench type: {}", bench_type);
    }

    // Create the parsing benchmark if "parse" or "full"
    if bench_type == "parse" || bench_type == "full" {
        let test_fn_name = format!("{}_parse_lex", fn_suffix);
        f.write_all(
            format!(
                r##"

fn {}(b: &mut Bencher) {{
    b.iter(|| {{ parse({:?}).ok() }});
}}

"##,
                test_fn_name, expr_string
            )
            .as_bytes(),
        )
        .expect("Error writing parse benchmark");
        all_tests.push(test_fn_name)
    }

    // Create the interpreter benchmark if "interpret" or "full"
    if bench_type == "interpret" || bench_type == "full" {
        let test_fn_name = format!("{}_interpret", fn_suffix);
        f.write_all(
            format!(
                r##"

fn {}(b: &mut Bencher) {{
    let data = Rcvar::new(Variable::from_json({:?}).expect("Invalid JSON given"));
    let expr = compile({:?}).unwrap();
    b.iter(|| {{ expr.search(&data).ok() }});
}}

"##,
                test_fn_name, given_string, expr_string
            )
            .as_bytes(),
        )
        .expect("Error writing interpret benchmark");
        all_tests.push(test_fn_name)
    }

    // Create the "full" benchmark if "full"
    if bench_type == "full" {
        let test_fn_name = format!("{}_full", fn_suffix);
        f.write_all(
            format!(
                r##"

fn {}(b: &mut Bencher) {{
    let data = Rcvar::new(Variable::from_json({:?}).expect("Invalid JSON given"));
    b.iter(|| {{ compile({:?}).unwrap().search(&data).ok() }});
}}

"##,
                test_fn_name, given_string, expr_string
            )
            .as_bytes(),
        )
        .expect("Error writing interpret benchmark");
        all_tests.push(test_fn_name)
    }
}

/// Generates a benchmark for a test case.
fn generate_test(
    filename: &str,
    suite_num: usize,
    case_num: usize,
    case: &serde_json::Map<String, Value>,
    given_string: &str,
    f: &mut File,
) {
    let fn_suffix = generate_fn_name(filename, suite_num, case_num, case);
    let case_string = serde_json::to_string(case).expect("Could not encode case");

    f.write_all(
        format!(
            r##"
#[test]
fn test_{}() {{
    let case: TestCase = TestCase::from_str({:?}).unwrap();
    let data = Rcvar::new(Variable::from_json({:?}).unwrap());
    case.assert({:?}, data).unwrap();
}}

"##,
            fn_suffix, case_string, given_string, filename
        )
        .as_bytes(),
    )
    .expect("Unable to write test");
}
