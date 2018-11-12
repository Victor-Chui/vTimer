#pragma once

#include <string>
#include <sstream>
#include <ctype.h>

namespace vTimer
{

class Common
{
public:
    template<typename T>
    static std::string tostr(const T& target);
    
    template<typename T>
    static T strto(const std::string &target);

    static std::string upper(const std::string &str);
    static std::string lower(const std::string &str);
};


template<typename T>
std::string Common::tostr(const T& target)
{
    std::stringstream ss; 
    ss << target;
    return ss.str();
}

template<typename T>
T Common::strto(const std::string &target)
{
    std::stringstream ss;
    ss << target;
    T result;
    ss >> result;
    return result;
}

std::string Common::upper(const std::string &str)
{
    std::string result = str;
    for (std::string::iterator it = result.begin(); it != result.end(); ++it)
        *it = toupper(*it);

    return result;
}

std::string Common::lower(const std::string &str)
{
    std::string result = str;
    for (std::string::iterator it = result.begin(); it != result.end(); ++it)
        *it = tolower(*it);

    return result;
}

} // namespace vTimer
