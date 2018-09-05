#pragma once
#include <xs.h>
#include <xs/unievent.h>
#include <xs/protocol/websocket.h>
#include <panda/unievent/websocket.h>

namespace xs { namespace unievent  { namespace websocket {

using namespace panda::unievent::websocket;

struct XSClient : Client, Backref {
    XSClient (Loop* loop = Loop::default_loop()) : ConnectionBase(loop), Client(loop) {}
private:
    ~XSClient () { Backref::dtor(); }
};

struct XSServerConnection : server::Connection, Backref {
    XSServerConnection (Server* server, uint64_t id) : ConnectionBase(server->loop()), Connection(server, id) {}
private:
    ~XSServerConnection () { Backref::dtor(); }
};

struct XSServer : Server, Backref {
    using Server::Server;

    server::ConnectionSP new_connection (uint64_t id) override {
        return new XSServerConnection(this, id);
    }

    static server::Location make_location      (const Hash& hvloc);
    static Server::Config   make_server_config (const Hash& hvcfg);
private:
    ~XSServer () { Backref::dtor(); }
};

}}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::websocket::Server*, TYPE> :
    TypemapObject<panda::unievent::websocket::Server*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {
    std::string package () { return "UniEvent::WebSocket::Server"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::ConnectionBase*, TYPE> : Typemap<panda::unievent::TCP*, TYPE> {
    //std::string package () = delete;
    std::string package () { return "UniEvent::WebSocket::ConnectionBase"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::server::Connection*, TYPE> : Typemap<panda::unievent::websocket::ConnectionBase*, TYPE> {
    std::string package () { return "UniEvent::WebSocket::Server::Connection"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Client*, TYPE> : Typemap<panda::unievent::websocket::ConnectionBase*, TYPE> {
    std::string package () { return "UniEvent::WebSocket::Client"; }
};

}
