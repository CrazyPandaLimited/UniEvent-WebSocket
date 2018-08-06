#pragma once
#include <xs/xs.h>
#include <xs/event.h>
#include <xs/websocket.h>
#include <panda/websocket/server.h>
#include <panda/websocket/Client.h>
#include <panda/websocket/server/Connection.h>

namespace xs { namespace websocket { namespace server {

using namespace panda::websocket::server;

struct XSConnection : Connection, Backref {
    using Connection::Connection;
private:
    ~XSConnection () { Backref::dtor(); }
};

struct XSServer : Server, Backref {
    using Server::Server;

    ConnectionSP new_connection (uint64_t id) override {
        return new XSConnection(this, id);
    }

    static Location     make_location      (const Hash& hvloc);
    static ServerConfig make_server_config (const Hash& hvcfg);
private:
    ~XSServer () { Backref::dtor(); }
};

}}}

namespace xs {

template <class TYPE> struct Typemap<panda::websocket::server::Server*, TYPE> :
    TypemapObject<panda::websocket::server::Server*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {
    std::string package () { return "Panda::WebSocket::Server"; }
};

template <class TYPE> struct Typemap<panda::websocket::BaseConnection*, TYPE> : Typemap<panda::event::TCP*, TYPE> {};

template <class TYPE> struct Typemap<panda::websocket::server::Connection*, TYPE> : Typemap<panda::websocket::BaseConnection*, TYPE> {
    std::string package () { return "Panda::WebSocket::Server::Connection"; }
};

template <class TYPE> struct Typemap<panda::websocket::Client*, TYPE> : Typemap<panda::websocket::BaseConnection*, TYPE> {
    std::string package () { return "Panda::WebSocket::Client"; }
};

}
