#include <stdlib.h>
#include <stddef.h>
#include <string.h>
// functions for parsing

const char *jassonpath_match_string(const char *begin, const char *end) {
    if (*begin == '\"')
        begin++;  // begin should be left quotation mark to match
    else
        return begin;
    for (; begin != end && *begin; begin++) {
        if (begin[0] == '\\' && begin[1] != '\0')
            begin += 2;
        else if (begin[0] == '\"') {
            begin++;
            break;
        }
    }
    return begin;
}

const char *jassonpath_next_delima(const char *begin, const char *end) {
    for (; begin != end && *begin; begin++) {
        if (begin[0] == '\"') begin = jassonpath_match_string(begin, end);
        if (begin[0] == '[' || begin[0] == ']' || begin[0] == '.') break;
    }
    return begin;
}

const char *jassonpath_next_matched_bracket(const char *begin, const char *end,
                                            char left, char right) {
    if (*begin == left)
        begin++;  // begin should be left bracket to match
    else
        return begin;
    int count = 1;
    for (; begin != end && *begin; begin++) {
        if (begin[0] == '\"') begin = jassonpath_match_string(begin, end);
        if (begin[0] == left)
            count++;
        else if (begin[0] == right)
            count--;
        if (!count) break;
    }
    return begin;
}

const char *jassonpath_next_seprator(const char *begin, const char *end,
                                     char sep) {
    for (; begin != end && *begin; begin++) {
        if (begin[0] == '\"') begin = jassonpath_match_string(begin, end);
        if (begin[0] == sep) break;
    }
    return begin;
}

const char *jassonpath_next_punctors_outside_para(const char *begin,
                                                  const char *end, char *sep) {
    for (; begin != end && *begin; begin++) {
        if (begin[0] == '\"') begin = jassonpath_match_string(begin, end);
        if (begin[0] == '(')
            begin = jassonpath_next_matched_bracket(begin, end, '(', ')');
        else if (strchr(sep, begin[0]))
            break;
    }
    return begin;
}

const char *jassonpath_next_punctor_outside_para(const char *begin,
                                                 const char *end, char sep) {
    for (; begin != end && *begin; begin++) {
        if (begin[0] == '\"') begin = jassonpath_match_string(begin, end);
        if (begin[0] == '(')
            begin = jassonpath_next_matched_bracket(begin, end, '(', ')');
        else if (begin[0] == sep)
            break;
    }
    return begin;
}

const char *jassonpath_strdup_no_terminal(const char *begin, const char *end) {
    size_t len = end - begin;
    if (!end) len = strlen(begin);
    char *ret = (char *)malloc(sizeof(char) * (len + 1));
    memcpy(ret, begin, len);
    ret[len] = '\0';
    return ret;
}
