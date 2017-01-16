#pragma once
#include <panda/string.h>

namespace panda { namespace websocket { namespace server {

using panda::string;

class Error {
public:
    Error (const string& what_arg) : _what(what_arg) {}
    virtual const string what () const { return _what; }
private:
    string _what;
};

class ConfigError : public Error {
    using Error::Error;
};

}}}
