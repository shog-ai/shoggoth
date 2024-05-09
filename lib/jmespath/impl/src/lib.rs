use jmespath;
use jmespath::Expression;
use jmespath::Variable;

use std::ffi::CStr;
use std::ffi::CString;
use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn jmespath_filter_json(json: *const c_char, filter: *const c_char) -> *mut c_char {
    let c_json: &CStr = unsafe { CStr::from_ptr(json) };
    let c_filter: &CStr = unsafe { CStr::from_ptr(filter) };

    let rust_json: &str = c_json.to_str().unwrap();
    let rust_filter: &str = c_filter.to_str().unwrap();

    let expr: Expression = jmespath::compile(rust_filter).unwrap();

    // Parse some JSON data into a JMESPath variable
    let data: Variable = jmespath::Variable::from_json(rust_json).unwrap();

    // Search the data with the compiled expression
    let result: String = expr.search(data).unwrap().to_string();

    let c_result: CString = CString::new(result).unwrap();
    return c_result.into_raw();
}
