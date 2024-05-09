#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "janssonpath.h"
#include "parse.h"

#define TRUE 1
#define FALSE 0

#define NEW(type) ((type *)malloc(sizeof(type)))
#define NEW_ARRAY(type, number) ((type *)malloc(sizeof(type) * (number)))
#define DELETE(obj) free((void *)obj)
#define DELETE_ARRAY(obj) free((void *)obj)

#define debug_out(json)                                               \
    do {                                                              \
        char *s = json_dumps(json, JSON_INDENT(2) | JSON_ENCODE_ANY); \
        if (s) {                                                      \
            puts(s);                                                  \
            free(s);                                                  \
        } else                                                        \
            puts("Empty");                                            \
    } while (0)

static json_t *json_path_eval(json_t *root, json_t *curr, const char *begin,
                              const char *end);

// all static functions here are internal implement. they follow the convention
// below: no check of arguments - pname/path must not be NULL; no accepting root
// note '$'; deal with [path, end) or from path till NULL encountered if
// end==NULL note that almost all jansson functions work fine with NULL input

static json_t *json_path_get_all_properties(json_t *json) {
    if (json_is_array(json))
        return json_incref(json);
    else if (json_is_object(json)) {
        json_t *ret = json_array();
        json_t *value;
        const char *key;
        json_object_foreach(json, key, value) { json_array_append(ret, value); }
        return ret;
    } else
        return NULL;
}

static path_result json_path_get_all_properties_col(path_result json) {
    path_result result = {NULL, TRUE};
    size_t index;
    json_t *iter;
    if (json.is_collection) {
        result.result = json_array();
        json_array_foreach(json.result, index, iter) {
            json_t *property = json_path_get_all_properties(iter);

            json_array_extend(result.result, property);
            json_decref(property);
        }
    } else {
        result.result = json_path_get_all_properties(json.result);
    }
    return result;
}

// remember to clean up stuffs before fail()
// note it can be recoverable fail - like property sellect on non-object when
// dealing with colleciton
#define fail() return result;

typedef enum logic_operator {
    logic_positive,
    logic_negtive,
    logic_and,
    logic_or
} logic_operator;

typedef enum comparison_operator {
    comparison_nocmp,
    comparison_eq,
    comparison_ne,
    comparison_gt,
    comparison_ge,
    comparison_lt,
    comparison_le,
    comparison_reg,  // yet not supported
    comparison_error
} comparison_operator;

typedef struct sub_path {  // sub_path does not own the string
    const char *begin;
    const char *end;
} sub_path;

typedef struct comparison {
    comparison_operator operator;
    sub_path oprand[2];  // number of oprand is defined by operator
} comparison;

struct logic_exp;  // forward declartion
typedef struct expression {
    union {
        struct logic_exp *logic;
        comparison *comp;
    };
    enum tag_t { exp_logic, exp_compare, exp_error } tag;
} expression;

typedef struct logic_exp {
    logic_operator operator;
    expression oprand[2];  // number of oprand is defined by operator
} logic_exp;

// static json_t* eval_sub(json_t *root, json_t * curr, expression* exp){
//   return NULL;
// }

#define remove_out_most_bar(begin, end)                                  \
    do {                                                                 \
        while (begin[0] == '(' && end[-1] == ')' &&                      \
               (jassonpath_next_matched_bracket(begin, end, '(', ')') == \
                (end - 1))) {                                            \
            begin++;                                                     \
            end--;                                                       \
        }                                                                \
    } while (0)

static comparison *compile_compare(const char *begin, const char *end) {
    comparison *result = NULL;
    remove_out_most_bar(begin, end);
    const char *comparison_first =
        jassonpath_next_punctors_outside_para(begin, end, "=<>!");
    result = NEW(comparison);
    if (comparison_first == end || !comparison_first[0]) {
        result->operator= comparison_nocmp;
        result->oprand[0].begin = begin;
        result->oprand[0].end = end;
        return result;
    }
    int len;
    // technically uesr can do something like (@.a>@.b)==@.t
    // however we ignore it for now
    // simple and dumb
    if (comparison_first[0] == '=' && comparison_first[1] == '=') {
        result->operator= comparison_eq;
        len = 2;
    } else if (comparison_first[0] == '!' && comparison_first[1] == '=') {
        result->operator= comparison_ne;
        len = 2;
    } else if (comparison_first[0] == '>' && comparison_first[1] != '=') {
        result->operator= comparison_gt;
        len = 1;
    } else if (comparison_first[0] == '>' && comparison_first[1] == '=') {
        result->operator= comparison_ge;
        len = 2;
    } else if (comparison_first[0] == '<' && comparison_first[1] != '=') {
        result->operator= comparison_lt;
        len = 1;
    } else if (comparison_first[0] == '<' && comparison_first[1] == '=') {
        result->operator= comparison_le;
        len = 2;
    } else if (comparison_first[0] == '=' && comparison_first[1] == '~') {
        result->operator= comparison_reg;
        len = 2;
    } else {
        result->operator= comparison_error;
        fail();
    }

    const char *lhss = begin, *lhse = comparison_first,
               *rhss = comparison_first + len, *rhse = end;
    remove_out_most_bar(lhss, lhse);
    remove_out_most_bar(rhss, rhse);
    result->oprand[0].begin = lhss;
    result->oprand[0].end = lhse;
    result->oprand[1].begin = rhss;
    result->oprand[1].end = rhse;
    return result;
}

static expression compile_expression(const char *begin, const char *end) {
    expression result = {{NULL}, exp_error};
    remove_out_most_bar(begin, end);
    const char *logic_first =
        jassonpath_next_punctors_outside_para(begin, end, "|&");
    // no binary logic operator found
    if (logic_first == end || !*logic_first) {
        // negative
        if (begin[0] == '!') {
            result.tag = exp_logic;
            result.logic = NEW(logic_exp);
            result.logic->operator= logic_negtive;
            result.logic->oprand[0] = compile_expression(begin + 1, end);
        } else {  // comparison
            result.tag = exp_compare;
            result.comp = compile_compare(begin, end);
        }
        return result;
    }
    // logic operator found
    if (logic_first[0] != logic_first[1]) fail();  //&& or ||
    result.tag = exp_logic;
    result.logic = NEW(logic_exp);
    result.logic->operator= logic_first[0] == '&' ? logic_and : logic_or;
    result.logic->oprand[0] = compile_expression(begin, logic_first);
    result.logic->oprand[1] = compile_expression(logic_first + 2, end);
    return result;
}

static void comparison_free(comparison *exp) { DELETE(exp); }

static void expression_free(expression exp);

static void logic_exp_free(logic_exp *exp) {
    expression_free(exp->oprand[0]);
    if (exp->operator> logic_negtive) expression_free(exp->oprand[1]);
    DELETE(exp);
}

static void expression_free(expression exp) {
    if (exp.tag == exp_logic)
        logic_exp_free(exp.logic);
    else
        comparison_free(exp.comp);
}

static path_result json_path_get_property(json_t *curr, const char *begin,
                                          const char *end) {
    path_result result = {NULL, FALSE};
    if (begin[0] == '*') {  // select all members
        result.is_collection = TRUE;
        if (begin[1] && begin + 1 != end) {
            result.result = json_array();
            fail();
        }
        result.result = json_path_get_all_properties(curr);
        return result;
    } else if (begin[0] == '.') {
        if (begin[1] && begin + 1 != end) fail();
        result.result = json_array();
        result.is_collection = TRUE;
        path_result out_layer = {json_array(), TRUE};
        json_array_append(out_layer.result, curr);

        json_array_append(result.result, curr);  // including itself

        while (out_layer.result && !(json_is_array(out_layer.result) &&
                                     !json_array_size(out_layer.result))) {
            path_result cur_layer = json_path_get_all_properties_col(out_layer);
            json_decref(out_layer.result);
            json_array_extend(result.result, cur_layer.result);
            out_layer = cur_layer;
        }

        json_decref(out_layer.result);
        return result;
    } else if (begin[0] == '#') {
        if (begin[1] && begin + 1 != end) fail();

        size_t len;
        if (json_is_array(curr))
            len = json_array_size(curr);
        else
            len = 0;
        result.result = json_integer(len);
        result.is_collection = FALSE;
        return result;
    }

    // number or named property
    const char *seg[4];
    seg[0] = begin;
    size_t segn = 1;  // number of parameters seprated by ':'
    while (segn < 4 && seg[segn - 1] != end && seg[segn - 1][0]) {
        seg[segn] = jassonpath_next_seprator(seg[segn - 1], end, ':');
        if (seg[segn] == end || !*seg[segn])  // not found
            break;
        else {
            seg[segn]++;
            segn++;
        }
    }
    seg[segn] = end;
    long seg_int[3];
    int seg_filled[3];  // is it a number and been filled?
    {  // make a scope to isolate i. workaround for poor supported inline
       // declaration in for
        size_t i;
        for (i = 0; i < segn; i++) {
            char *end_of_int;
            if ((seg[i] != end && seg[i][0])) {
                seg_int[i] = strtol(seg[i], &end_of_int, 10);
                seg_filled[i] = (end_of_int != seg[i]);
            } else
                seg_filled[i] = FALSE;
            // if(seg_filled[i]&&end_of_int!=seg[i+1]) ; error, but we ignore
            // that
        }
    }
    // only one parameter and it's not a number
    if (segn == 1 && !seg_filled[0]) {  // select named properties
        result.is_collection = FALSE;
        if (!json_is_object(curr)) fail();
        const char *pname = jassonpath_strdup_no_terminal(begin, end);
        result.result = json_incref(json_object_get(curr, pname));
        free((void *)pname);
    } else if (segn == 1 && seg_filled[0]) {  //[i] and [-i]
        result.is_collection = FALSE;
        if (!json_is_array(curr) || !seg_filled[0]) fail();
        size_t index;
        if (seg_int[0] >= 0 && seg[0][0] != '-')
            index = seg_int[0];
        else
            index = json_array_size(curr) - (-seg_int[0] + 1);
        result.result = json_incref(json_array_get(curr, index));
    } else {  //[i:j]([:j] [i:]) or [i:j:k] or [-i:]
        result.is_collection = TRUE;
        result.result = json_array();
        if (!json_is_array(curr)) fail();  // incorrect input
        long step = 1;
        size_t index_begin = 0;
        size_t index_end = json_array_size(curr);
        int rev =
            (segn == 2 && seg_filled[0] && !seg_filled[1] && seg_int[0] < 0);
        if (!rev) {
            if (seg_filled[0] && seg_int[0] >= 0) index_begin = seg_int[0];
            if (segn == 2 && seg_filled[1] && ((size_t)seg_int[1]) < index_end)
                index_end = seg_int[1];
            else if (segn == 3 && seg_filled[2] &&
                     ((size_t)seg_int[2]) < index_end)
                index_end = seg_int[2];
            if (segn == 3 && seg_filled[1] && seg_filled[1] != 0)
                step = seg_int[1];
            // negative step does not mean any thing
            // because we do not arrange output with order
            if (step < 0) step = -step;
        } else {
            long index_begin_l =
                index_end + seg_int[0];  // index_begin=index_end-i
            if (index_begin_l >= 0) index_begin = index_begin_l;
        }
        size_t i;
        for (i = index_begin; i < index_end; i += step) {
            json_t *ele = json_array_get(curr, i);
            json_array_append_new(result.result, ele);
        }
    }
    return result;
}

static json_t *json_index_json(json_t *json, const json_t *index) {
    json_t *sel;
    size_t i = 0;
    int named = FALSE;
    if (json_is_integer(index)) {
        i = json_integer_value(index);
    } else if (json_is_true(index)) {
        i = 1;
    } else if (json_is_false(index) || json_is_null(index)) {
        i = 0;
    } else if (json_is_string(index)) {
        named = TRUE;
    }
    if (named)
        sel = json_object_get(json, json_string_value(index));
    else
        sel = json_array_get(json, i);
    return sel;
}

static int json_to_bool(json_t *json) {
    if (!json) return FALSE;
    return json_is_true(json) ||
           (json_is_integer(json) && json_integer_value(json)) ||
           (json_is_string(json) && !strcmp("true", json_string_value(json)));
}

#define OCT_DIGIT_NOTE (CHAR_MAX)
#define HEX_DIGIT_NOTE (CHAR_MAX - 1)

static char unescape_map[UCHAR_MAX] = {
    ['a'] = '\a',
    ['b'] = '\b',
    ['f'] = '\f',
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['\''] = '\'',
    ['"'] = '"',
    ['?'] = '?',
    ['\\'] = '\\',

    ['0'] = OCT_DIGIT_NOTE,
    OCT_DIGIT_NOTE,
    OCT_DIGIT_NOTE,
    OCT_DIGIT_NOTE,
    OCT_DIGIT_NOTE,
    OCT_DIGIT_NOTE,
    OCT_DIGIT_NOTE,
    OCT_DIGIT_NOTE,  // 0-7
    ['x'] = HEX_DIGIT_NOTE,
};

static const char *unescape(const char *begin, const char *end) {
    char *ret = NEW_ARRAY(char, end - begin + 1);
    char *iter = ret;
    const char *src_iter;
    for (src_iter = begin; src_iter != end && *src_iter; ++iter) {
        if (*src_iter != '\\') {
            *iter = *src_iter;
            ++src_iter;
        } else {
            ++src_iter;
            if (src_iter == end) {
                DELETE_ARRAY(ret);
                return NULL;
            }
            char maped = unescape_map[(unsigned char)*src_iter];
            switch (maped) {
            case '\0': {
                DELETE_ARRAY(ret);
                return NULL;
            }
            case OCT_DIGIT_NOTE: {
                char oct = 0;
                int count = 0;
                while (src_iter != end &&
                       unescape_map[(unsigned char)*src_iter] ==
                           OCT_DIGIT_NOTE &&
                       count < 3) {
                    oct *= 8;
                    oct += *src_iter - '0';
                    ++src_iter;
                    ++count;
                }
                *iter = oct;
            } break;
            case HEX_DIGIT_NOTE: {
                ++src_iter;
                if (src_iter == end) {
                    DELETE_ARRAY(ret);
                    return NULL;
                }  //'x'
                char hex = 0;
                int count = 0;
                while (src_iter != end && isxdigit(*src_iter) && count < 2) {
                    hex *= 16;
                    int digit;
                    if (isdigit(*src_iter))
                        digit = *src_iter - '0';
                    else if (*src_iter > 'a' && *src_iter < 'f')
                        digit = *src_iter - 'a';
                    else if (*src_iter > 'A' && *src_iter < 'F')
                        digit = *src_iter - 'A';
                    else
                        assert(0);
                    hex += digit;
                    ++src_iter;
                    ++count;
                }
                if (count == 0) {
                    DELETE_ARRAY(ret);
                    return NULL;
                }
                *iter = hex;
            } break;
            default:
                *iter = maped;
                ++src_iter;
            }
        }
    }
    *iter = '\0';
    return ret;
}

#undef OCT_DIGIT_NOTE
#undef HEX_DIGIT_NOTE

static json_t *json_literal_from_string(const char *begin, const char *end) {
    json_t *result = NULL;
    if (begin[0] == '\"' && end[-1] == '\"' && (end - begin) > 1) {
        const char *value_str = unescape(begin + 1, end - 1);
        result = json_string(value_str);
        DELETE_ARRAY(value_str);
    } else if (isdigit(begin[0]) || begin[0] == '-') {
        char *end_of_number;
        long ret = strtol(begin, &end_of_number, 10);
        if (end == end_of_number || !*end_of_number)
            result = json_integer(ret);
        else {
            double retd = strtod(begin, &end_of_number);
            if (end == end_of_number || !*end_of_number)
                result = json_real(retd);
        }
    } else {
        const char *str = jassonpath_strdup_no_terminal(begin, end);
        if (!strcmp("true", str))
            result = json_true();
        else if (!strcmp("false", str))
            result = json_false();
        else if (!strcmp("null", str))
            result = json_null();
        DELETE_ARRAY(str);
    }
    return result;
}

static path_result json_path_get_impl(json_t *root, path_result curr,
                                      const char *path, const char *end);

static json_t *json_path_eval(json_t *root, json_t *curr, const char *begin,
                              const char *end) {
    if (begin[0] != '$' && begin[0] != '@' && begin[0] != '.') {
        return json_literal_from_string(begin, end);
    };
    // sub path
    path_result curr_p = {curr, FALSE};
    path_result root_p = {root, FALSE};
    if (begin[0] == '@')
        return json_path_get_impl(root, curr_p, begin + 1, end).result;
    else if (begin[0] == '$')
        return json_path_get_impl(root, root_p, begin + 1, end).result;
    else
        return json_path_get_impl(root, curr_p, begin, end)
            .result;  // default to current node
}

static double json_to_double(const json_t *json) {
    if (json_is_real(json))
        return json_real_value(json);
    else if (json_is_string(json)) {
        const char *begin = json_string_value(json);
        char *end;
        double value = strtod(begin, &end);
        if (end == begin + json_string_length(json))
            return value;
        else
            return NAN;
    } else if (json_is_integer(json)) {
        return json_integer_value(json);
    } else if (json_is_boolean(json)) {
        return json_boolean_value(json) ? 1.0 : 0.0;
    } else if (json_is_null(json)) {
        return 0.0;
    }
    // else if(json_is_object(json)||json_is_array(json))
    return NAN;
}

static long long json_to_int(const json_t *json) {
    static const long long error = (long long)(INT_MAX) + 1;
    if (json_is_real(json))
        return json_real_value(json);
    else if (json_is_string(json)) {
        const char *begin = json_string_value(json);
        char *end;
        long value = strtol(begin, &end, 10);
        if (end == begin + json_string_length(json))
            return value;
        else
            return error;
    } else if (json_is_integer(json)) {
        return json_integer_value(json);
    } else if (json_is_boolean(json)) {
        return json_boolean_value(json) ? 1 : 0;
    } else if (json_is_null(json)) {
        return 0;
    }
    // else if(json_is_object(json)||json_is_array(json))
    return error;
}

int comp_diff(double diff, comparison_operator op) {
    int result;
    if (!diff)
        result = (op == comparison_eq) || (op == comparison_ge) ||
                 (op == comparison_le);
    else if (diff > 0.0)
        result = (op == comparison_ne) || (op == comparison_gt) ||
                 (op == comparison_ge);
    else
        result = (op == comparison_ne) || (op == comparison_lt) ||
                 (op == comparison_le);
    return result;
}

static json_t *json_compare(comparison_operator op, json_t *lhs, json_t *rhs) {
    if (op == comparison_nocmp || op == comparison_error ||
        op == comparison_reg)
        return NULL;  // should not happen
    if (!lhs || !rhs) return json_false();
    if (json_is_array(lhs) || json_is_array(rhs) || json_is_object(lhs) ||
        json_is_object(rhs))
        return json_false();

    if (json_is_null(lhs) ||
        json_is_null(
            rhs)) {  // null should not be equal to anything other than null
        if (op != comparison_eq && op != comparison_ne) return json_false();
        return json_boolean((json_is_null(lhs) && json_is_null(rhs)) ==
                            (op == comparison_eq));
    }
    // conversion: string->boolean->integer->number string->integer->number
    // however "true" should not be treat as 1
    if (json_is_number(lhs) || json_is_number(rhs) ||
        (op != comparison_eq && op != comparison_ne)) {  // numeric compare
        if (json_is_real(lhs) || json_is_real(rhs)) {
            double lhs_d = json_to_double(lhs);
            double rhs_d = json_to_double(rhs);
            if (isnan(lhs_d) || isnan(rhs_d)) return json_false();
            return json_boolean(comp_diff(lhs_d - rhs_d, op));
        } else {
            long lhs_d = json_to_int(lhs);
            long rhs_d = json_to_int(rhs);
            if (lhs_d > INT_MAX || rhs_d > INT_MAX) return json_false();
            return json_boolean(comp_diff(lhs_d - rhs_d, op));
        }
    } else if (json_is_boolean(lhs) || json_is_boolean(rhs)) {
        int lhs_b = json_to_bool(lhs);
        int rhs_b = json_to_bool(rhs);
        lhs_b = lhs_b ? 1 : 0;
        rhs_b = rhs_b ? 1 : 0;  // normalization
        return json_boolean((lhs_b == rhs_b) == (op == comparison_eq));
    } else if (json_is_string(lhs) && json_is_string(rhs)) {  // string compare
        int a = strcmp(json_string_value(lhs), json_string_value(rhs));
        return json_boolean(a == (op == comparison_ne));
    }
    return json_true();
}

static json_t *execute_compare(comparison *comp, json_t *root, json_t *curr) {
    json_t *result = NULL;
    switch (comp->operator) {
    case comparison_nocmp:
        result = json_path_eval(root, curr, comp->oprand[0].begin,
                                comp->oprand[0].end);
        break;
    case comparison_eq:
    case comparison_ne:
    case comparison_gt:
    case comparison_ge:
    case comparison_lt:
    case comparison_le: {
        json_t *lhs = json_path_eval(root, curr, comp->oprand[0].begin,
                                     comp->oprand[0].end);
        json_t *rhs = json_path_eval(root, curr, comp->oprand[1].begin,
                                     comp->oprand[1].end);
        result = json_compare(comp->operator, lhs, rhs);
        json_decref(lhs);
        json_decref(rhs);
    } break;
    case comparison_reg:  // yet not supported
    case comparison_error:
    default:
        result = NULL;
    }
    return result;
}

static json_t *execute_exp(expression exp, json_t *root, json_t *curr) {
    if (exp.tag == exp_logic) {
        logic_exp *logic = exp.logic;
        if (logic->operator== logic_positive || logic->operator==
            logic_negtive) {
            json_t *ret = execute_exp(logic->oprand[0], root, curr);
            int b = json_to_bool(ret);
            json_decref(ret);
            if (logic->operator== logic_negtive) b = !b;
            return json_boolean(b);
        } else if (logic->operator== logic_and || logic->operator== logic_or) {
            json_t *ret1 = execute_exp(logic->oprand[0], root, curr),
                   *ret2 = execute_exp(logic->oprand[1], root, curr);
            int b1 = json_to_bool(ret1), b2 = json_to_bool(ret2);
            json_decref(ret1);
            json_decref(ret2);
            int r;
            if (logic->operator== logic_and)
                r = b1 && b2;
            else
                r = b1 || b2;
            return json_boolean(r);
        }
    } else if (exp.tag == exp_compare) {
        return execute_compare(exp.comp, root, curr);
    }
    return NULL;
}

static path_result json_path_get_property_col(json_t *root, path_result curr,
                                              const char *name,
                                              const char *end) {
    path_result result = {NULL, FALSE};
    size_t index;
    json_t *iter;

    int cond = (name[0] == '?');
    if (name[0] == '(' ||
        (cond && name[1] == '(')) {  // expression [?(..)] [(..)]
        if (cond) name++;
        const char *right =
            jassonpath_next_matched_bracket(name, end, '(', ')');
        if (right != end - 1) fail();
        expression exp = compile_expression(name, end);

        if (exp.tag == exp_error) fail();

        if (cond) {
            json_t *properties = json_path_get_all_properties_col(curr)
                                     .result;  // unify properties into array

            result.result = json_array();
            result.is_collection = TRUE;
            json_array_foreach(properties, index, iter) {
                json_t *exc_result = execute_exp(exp, root, iter);
                int selected = json_to_bool(exc_result) ||
                               (json_is_string(exc_result) &&
                                json_string_length(
                                    exc_result));  // special case for path like
                                                   // "$.store.book[?(@.isbn)]"
                if (selected) json_array_append(result.result, iter);
                json_decref(exc_result);
            }
            json_decref(properties);
        } else {
            if (curr.is_collection) {
                result.result = json_array();
                result.is_collection = TRUE;
                json_array_foreach(curr.result, index, iter) {
                    json_t *exc_result = execute_exp(exp, root, iter);
                    json_t *sel = json_index_json(iter, exc_result);
                    json_array_append(result.result, sel);
                    json_decref(sel);
                    json_decref(exc_result);
                }
            } else {
                json_t *exc_result = execute_exp(exp, root, curr.result);
                result.result = json_index_json(curr.result, exc_result);
                result.is_collection = FALSE;
                json_decref(exc_result);
            }
        }

        expression_free(exp);
        return result;
    }

    if (curr.is_collection) {
        result.result = json_array();
        json_array_foreach(curr.result, index, iter) {
            path_result r = json_path_get_property(iter, name, end);
            if (r.is_collection)
                json_array_extend(result.result, r.result);
            else
                json_array_append(result.result, r.result);
            json_decref(r.result);
        }
        result.is_collection = TRUE;
    } else {
        result = json_path_get_property(curr.result, name, end);
    }
    return result;
}

// on_collection if we are path_get on a collection
// we treat . and [] the same way as property
// and by this we can simplify the implementation a lot
static path_result json_path_get_impl(json_t *root, path_result curr,
                                      const char *path, const char *end) {
    if (!curr.result || path == end ||
        *path == '\0') {  // no selection to do. return original collection
        json_incref(
            curr.result);  // note this should looks like a copy to users
        return curr;
    }
    path_result result = {NULL, FALSE};

    if (!(path[0] == '.' || path[0] == '[')) fail();

    const char *pname = path + 1;
    const char *pname_end;
    // got the full property name
    if (path[0] == '[') {
        pname_end = jassonpath_next_matched_bracket(path, end, '[', ']');
        if (pname_end[0] != ']') fail();
        path = pname_end + 1;
    } else if (path[1] == '.') {
        pname_end = path + 2;
        path = pname_end;
    } else {
        pname_end = jassonpath_next_delima(pname, end);
        path = pname_end;
    }

    const char *name_begin, *name_end;
    // if it's string, deal with the string problems
    if (*pname == '\"') {
        if (pname_end == pname || pname_end[-1] != '\"') fail();
        pname++;
        pname_end--;
        name_begin = unescape(pname, pname_end);
    } else {
        name_begin = jassonpath_strdup_no_terminal(pname, pname_end);
    }
    name_end = name_begin + (pname_end - pname);

    result = json_path_get_property_col(root, curr, name_begin, name_end);
    DELETE_ARRAY(name_begin);
    path_result ret = json_path_get_impl(root, result, path, end);
    json_decref(result.result);
    return ret;
}

// json_path_get,
// version with distinction between collection and simple result
path_result json_path_get_distinct(json_t *json, const char *path) {
    path_result result = {NULL, FALSE};
    if (!path || !json) fail();
    if (path[0] == '$')
        path++;  // it can be omited safely since current node is the root node
    // note that every json path selection is applyed to a collection of json
    // nodes
    path_result cur = {json, FALSE};
    result = json_path_get_impl(json, cur, path, NULL);
    return result;
}

// return NULL if either of arguments is NULL or any error occurs.
// path must be at least zero length and terminated with '\0'
// assuming no extra spaces except for string
// users are reponsible to decref the json_node returned
// extention to standard jsonpath - $."0" $.0 $[0] $["0"] they are all the
// same(for simplify implementation)
json_t *json_path_get(json_t *json, const char *path) {
    return json_path_get_distinct(json, path).result;
}