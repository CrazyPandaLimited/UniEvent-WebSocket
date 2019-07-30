#pragma once
#include <xs/unievent.h>
#include <xs/protocol/websocket.h>
#include <panda/unievent/websocket.h>

namespace xs { namespace unievent  { namespace websocket {
    using namespace panda::unievent::websocket;
    using panda::unievent::LoopSP;

    struct XSClient : Client, Backref {
        XSClient (const LoopSP& loop, const Client::Config& config) : Connection(loop), Client(loop, config) {}
    private:
        ~XSClient () { Backref::dtor(); }
    };

    struct XSServerConnection : ServerConnection, Backref {
        XSServerConnection (Server* server, uint64_t id, const Config& conf) : Connection(server->loop()), ServerConnection(server, id, conf) {}
    private:
        ~XSServerConnection () { Backref::dtor(); }
    };

    struct XSConnectionIterator {
        XSConnectionIterator (const Server::Connections& connections) {
            cur = connections.begin();
            end = connections.end();
        }

        Scalar next() {
            if (cur == end) return Scalar::undef;
            Scalar res = xs::out(cur->second);
            ++cur;
            return res;
        }

    private:
        using iterator = Server::Connections::const_iterator;

        iterator cur;
        iterator end;
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
    static std::string package () { return "UniEvent::WebSocket::Server"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Listener*, TYPE> : Typemap<panda::unievent::Tcp*, TYPE> {
    static std::string package () { return "UniEvent::WebSocket::Listener"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Connection*, TYPE> : Typemap<panda::unievent::Tcp*, TYPE> {
    static std::string package () { throw "can't return abstract class without backref"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::ServerConnection*, TYPE> : Typemap<panda::unievent::websocket::Connection*, TYPE> {
    static std::string package () { return "UniEvent::WebSocket::ServerConnection"; }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Client*, TYPE> : Typemap<panda::unievent::websocket::Connection*, TYPE> {
    static std::string package () { return "UniEvent::WebSocket::Client"; }
};

template <class TYPE> struct Typemap<xs::unievent::websocket::XSConnectionIterator*, TYPE> :
    TypemapObject<xs::unievent::websocket::XSConnectionIterator*, TYPE, ObjectTypePtr, ObjectStorageMG, StaticCast>
{
    static std::string package () { return "UniEvent::WebSocket::XSConnectionIterator"; }
};

template <> struct Typemap<panda::unievent::websocket::Location> : TypemapBase<panda::unievent::websocket::Location> {
    using Location = panda::unievent::websocket::Location;
    static Location in (pTHX_ SV* arg) {
        const Hash h = arg;
        Scalar val;
        Location loc;
        if ((val = h["host"]))       loc.host       = xs::in<panda::string>(val);
        if ((val = h["port"]))       loc.port       = Simple(val);
        if ((val = h["name"]))       loc.name       = xs::in<panda::string>(val);
        if ((val = h["ssl_ctx"]))    loc.ssl_ctx    = xs::in<SSL_CTX*>(val);
        if ((val = h["backlog"]))    loc.backlog    = Simple(val);
        if ((val = h["reuse_port"])) loc.reuse_port = val.is_true();
        return loc;
    }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Connection::Config, TYPE> : Typemap<panda::protocol::websocket::Parser::Config, TYPE> {
    static TYPE in (pTHX_ SV* arg) {
        using Super = Typemap<panda::protocol::websocket::Parser::Config, TYPE>;
        TYPE cfg = Super::in(aTHX_ arg);
        return cfg;
    }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Server::Config, TYPE> : TypemapBase<panda::unievent::websocket::Server::Config, TYPE> {
    using Location = panda::unievent::websocket::Location;
    static TYPE in (pTHX_ SV* arg) {
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
