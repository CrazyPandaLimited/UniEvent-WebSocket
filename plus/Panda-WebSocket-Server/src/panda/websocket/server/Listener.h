#pragma once
#include <panda/string.h>
#include <panda/event/TCP.h>

namespace panda { namespace websocket { namespace server {

using panda::string;
using panda::event::TCP;
using panda::event::Loop;

struct Location {
    string   host;
    uint16_t port;
    bool     secure;     // WSS if true
    bool     reuse_port; // several listeners(servers) can be bound to the same port if true, useful for threaded apps
    int      backlog;    // max accept queue
    SSL_CTX* ssl_ctx;    // config with sertificate for server
};

class Listener : public TCP {
    Location _location;

public:
    Listener (Loop* loop, const Location& loc);

    const Location& location () const { return _location; }

    void run ();
};

typedef shared_ptr<Listener> ListenerSP;

template <typename Stream>
Stream& operator << (Stream& stream, const panda::websocket::server::Location& loc) {
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

