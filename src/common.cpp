#include "common.hpp"

#include <algorithm>
#include <locale>

using namespace std;

string jeff::join(list<string> entries, function<string(string, string)> join_lines) {
    return join(entries, string(""), join_lines);
}

string jeff::join(list<string> entries, string start, function<string(string, string)> join_lines) {
    return accumulate(entries.begin(), entries.end(), start, join_lines);
}

string jeff::join(list<bool> entries, function<string(string, string)> join_values) {
    return join(entries, string(""), join_values);
}

string jeff::join(list<bool> entries, string start, function<string(string, string)> join_values) {
    function<string(bool)> transform_values = [](bool b) -> string { return (b == 0) ? "false" : "true";};
    list<string> values;
    transform(entries.begin(), entries.end(), back_inserter(values), transform_values);
    return join(values, start, join_values);
}

wstring jeff::L(const string &str) {
    wstring ret;
    copy(str.begin(), str.end(), back_inserter(ret));
    return ret;
}

string jeff::S(const wstring &str) {
    locale const loc("");
    wchar_t const *from = str.c_str();
    size_t const len = str.size();
    vector<char> buffer(len + 1);
    std::use_facet<std::ctype<wchar_t> >(loc).narrow(from, from + len, '_', &buffer[0]);
    return string(&buffer[0], &buffer[len]);
}
/*
inline string jeff::S(const wstring &str) {
    string ret;
    copy(str.begin(), str.end(), back_inserter(ret));
    return ret;
}*/
