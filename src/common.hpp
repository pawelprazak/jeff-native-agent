#ifndef JEFF_NATIVE_AGENT_COMMON_H
#define JEFF_NATIVE_AGENT_COMMON_H

#include <functional>
#include <numeric>
#include <list>
#include <string>

namespace jeff {
    std::string join(std::list<std::string> entries, std::function<std::string(std::string, std::string)> join_lines);

    std::string join(std::list<std::string> entries, std::string start,
                     std::function<std::string(std::string, std::string)> join_lines);

    std::string join(std::list<bool> entries, std::function<std::string(std::string, std::string)> join_values);

    std::string join(std::list<bool> entries, std::string start,
                     std::function<std::string(std::string, std::string)> join_values);

    std::wstring L(const std::string &str);

    std::string S(const std::wstring &str);
}
#endif //JEFF_NATIVE_AGENT_COMMON_H
