#include "common.hpp"
#include <locale>
#include <vector>

using namespace std;

string jeff::join(list<string> entries, function<string(string, string)> join_lines) {
    return join(entries, string(""), join_lines);
}

string jeff::join(list<string> entries, string start, function<string(string, string)> join_lines) {
    return accumulate(entries.begin(), entries.end(), start, join_lines);
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
