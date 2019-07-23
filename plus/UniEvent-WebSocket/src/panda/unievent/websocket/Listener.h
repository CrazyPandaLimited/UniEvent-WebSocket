#pragma once
#include <panda/string.h>
#include <panda/unievent/Tcp.h>

namespace panda { namespace unievent { namespace websocket {

using panda::string;

struct Location {
    string   host;
    uint16_t port       = 0;
    bool     reuse_port = true;    // several listeners(servers) can be bound to the same port if true, useful for threaded apps
    int      backlog    = 4096;    // max accept queue
    SSL_CTX* ssl_ctx    = nullptr; // config with sertificate for server
};

struct Listener : Tcp {
    Listener (const LoopSP& loop, const Location& loc);

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
    stream << ",secure:" << bool(loc.ssl_ctx);
    stream << ",reuse_port:" << loc.reuse_port;
    stream << ",backlog:" << loc.backlog;
    stream << "}";
    return stream;
}

}}}
