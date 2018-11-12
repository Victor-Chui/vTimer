#pragma once

#include <string>
#include <stdexcept>
#include <errno.h>

namespace vTimer
{

class Exception : public std::exception
{
public:
    Exception(const std::string &buffer)
    : _buffer(buffer), _errCode(0) {}

    Exception(const std::string &buffer, int errCode)
    : _buffer(buffer), _errCode(errCode) {}

    virtual ~Exception() throw() {}

    const char* what() const throw()
    {
        return _buffer.c_str();
    }

    int getErrCode() const
    {
        return _errCode;
    }

private:
    std::string     _buffer;
    int             _errCode;
};

} // namespace vTimer
