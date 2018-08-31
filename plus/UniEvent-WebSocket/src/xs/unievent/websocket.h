#pragma once
#include <xs.h>
#include <xs/unievent.h>
#include <xs/protocol/websocket.h>
#include <panda/unievent/websocket.h>

namespace xs { namespace unievent  { namespace websocket {

using namespace panda::unievent::websocket;

struct XSClient : Client, Backref {
    XSClient (Loop* loop = Loop::default_loop()) : Connection(loop), Client(loop) {}
private:
    ~XSClient () { Backref::dtor(); }
};

struct XSServerConnection : ServerConnection, Backref {
    XSServerConnection (Server* server, uint64_t id) : Connection(server->loop()), ServerConnection(server, id) {}
private:
    ~XSServerConnection () { Backref::dtor(); }
};

struct XSServer : Server, Backref {
    using Server::Server;

    ServerConnectionSP new_connection (uint64_t id) override {
        return new XSServerConnection(this, id);
    }

    static Location     make_location      (const Hash& hvloc);
    static ServerConfig make_server_config (const Hash& hvcfg);
private:
    ~XSServer () { Backref::dtor(); }
};

}}}

namespace xs {

template <class TYPE> struct Typemap<panda::unievent::websocket::Server*, TYPE> :
    TypemapObject<panda::unievent::websocket::Server*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMGBackref, DynamicCast> {
    std::string package () { return "UniEvent::WebSocket::Server"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Connection*, TYPE> : Typemap<panda::unievent::TCP*, TYPE> {
    //std::string package () = delete;
    std::string package () { return "UniEvent::WebSocket::Connection"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::ServerConnection*, TYPE> : Typemap<panda::unievent::websocket::Connection*, TYPE> {
    std::string package () { return "UniEvent::WebSocket::ServerConnection"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Client*, TYPE> : Typemap<panda::unievent::websocket::Connection*, TYPE> {
    std::string package () { return "UniEvent::WebSocket::Client"; }
};

}
