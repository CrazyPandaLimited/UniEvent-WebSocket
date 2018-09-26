#pragma once
#include <xs.h>
#include <xs/unievent.h>
#include <xs/protocol/websocket.h>
#include <panda/unievent/websocket.h>

namespace xs { namespace unievent  { namespace websocket {
    using namespace panda::unievent::websocket;

    struct XSClient : Client, Backref {
        XSClient (Loop* loop, const Client::Config& config) : Connection(loop), Client(loop, config) {}
    private:
        ~XSClient () { Backref::dtor(); }
    };

    struct XSServerConnection : ServerConnection, Backref {
        XSServerConnection (Server* server, uint64_t id, const Config& conf) : Connection(server->loop()), ServerConnection(server, id, conf) {}
    private:
        ~XSServerConnection () { Backref::dtor(); }
    };

    struct XSServer : Server, Backref {
        using Server::Server;

        ServerConnectionSP new_connection (uint64_t id) override { return new XSServerConnection(this, id, conn_conf); }

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
    std::string package () { throw "can't return abstract class without backref"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::ServerConnection*, TYPE> : Typemap<panda::unievent::websocket::Connection*, TYPE> {
    std::string package () { return "UniEvent::WebSocket::ServerConnection"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Client*, TYPE> : Typemap<panda::unievent::websocket::Connection*, TYPE> {
    std::string package () { return "UniEvent::WebSocket::Client"; }
};

template <> struct Typemap<panda::unievent::websocket::Location> : TypemapBase<panda::unievent::websocket::Location> {
    using Location = panda::unievent::websocket::Location;
    Location in (pTHX_ SV* arg) {
        const Hash h = arg;
        Scalar val;
        Location loc;
        if ((val = h["host"]))       loc.host       = xs::in<panda::string>(val);
        if ((val = h["port"]))       loc.port       = Simple(val);
        if ((val = h["secure"]))     loc.secure     = val.is_true();
        if ((val = h["ssl_ctx"]))    loc.ssl_ctx    = xs::in<SSL_CTX*>(val);
        if ((val = h["backlog"]))    loc.backlog    = Simple(val);
        if ((val = h["reuse_port"])) loc.reuse_port = val.is_true();
        return loc;
    }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Connection::Config, TYPE> : TypemapBase<panda::unievent::websocket::Connection::Config, TYPE> {
    TYPE in (pTHX_ SV* arg) {
        const Hash h = arg;
        TYPE cfg; Scalar val;
        if ((val = h["max_handshake_size"])) cfg.max_handshake_size = Simple(val);
        if ((val = h["max_frame_size"]))     cfg.max_frame_size     = Simple(val);
        if ((val = h["max_message_size"]))   cfg.max_message_size   = Simple(val);
        return cfg;
    }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Server::Config, TYPE> : TypemapBase<panda::unievent::websocket::Server::Config, TYPE> {
    using Location = panda::unievent::websocket::Location;
    TYPE in (pTHX_ SV* arg) {
        const Hash h = arg;
        TYPE cfg;

        Scalar val = h["locations"];
        if (val) {
            const auto list = xs::in<Array>(val);
            for (const auto& elem : list) {
                if (!elem.is_hash_ref()) throw "location should be a hash";
                cfg.locations.push_back(xs::in<Location>(elem));
            }
        }

        if ((val = h["connection"])) cfg.connection = xs::in<decltype(cfg.connection)>(val);

        return cfg;
    }
};

}
