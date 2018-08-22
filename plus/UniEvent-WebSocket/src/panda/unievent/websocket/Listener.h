#pragma once
#include <panda/string.h>
#include <panda/unievent/TCP.h>

namespace panda { namespace unievent { namespace websocket {

using panda::string;

struct Location {
    string   host;
    uint16_t port;
    bool     secure;     // WSS if true
    bool     reuse_port; // several listeners(servers) can be bound to the same port if true, useful for threaded apps
    int      backlog;    // max accept queue
    SSL_CTX* ssl_ctx;    // config with sertificate for server
};

struct Listener : TCP {
    Listener (Loop* loop, const Location& loc);

    const Location& location () const { return _location; }

    void run ();
private:
    Location _location;
};

using ListenerSP = iptr<Listener>;

template <typename Stream>
Stream& operator<< (Stream& stream, const Location& loc) {
    stream << "Location{";
    stream << "host:\"" << loc.host << "\"";
    stream << ",port:" << loc.port;
    stream << ",secure:" << loc.secure;
    stream << ",reuse_port:" << loc.reuse_port;
    stream << ",backlog:" << loc.backlog;
    stream << "}";
    return stream;
}

}}}

