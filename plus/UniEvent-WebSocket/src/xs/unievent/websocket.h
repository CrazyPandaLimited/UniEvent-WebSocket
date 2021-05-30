#pragma once
#include <xs/unievent.h>
#include <xs/protocol/websocket.h>
#include <panda/unievent/websocket.h>

namespace xs { namespace unievent  { namespace websocket {
    using namespace panda::unievent::websocket;
    using panda::unievent::LoopSP;

    inline uint64_t get_time (double val) { return val * 1000; }

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

        ServerConnectionSP new_connection (uint64_t id) override {
            ServerConnectionSP ret = new XSServerConnection(this, id, conn_conf);
            xs::out(ret); // fill backref
            return ret;
        }

    private:
        ~XSServer () { Backref::dtor(); }
    };

    inline ClientConnectRequestSP  make_request  (const Hash& params, const ClientConnectRequestSP& dest = {}) {
        ClientConnectRequestSP ret = dest ? dest : ClientConnectRequestSP(new ClientConnectRequest());
        xs::protocol::websocket::make_request(params, ret);

        Scalar val;

        if ((val = params.fetch("addr_hints")))      ret->addr_hints = xs::in<panda::unievent::AddrInfoHints>(val);
        if ((val = params.fetch("cached_resolver"))) ret->cached_resolver = SvTRUE(val);
        if ((val = params.fetch("timeout")))         ret->timeout.set(get_time(Simple(val)));

        return ret;
    }

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

template <class TYPE> struct Typemap<unievent::websocket::Connection::Statistics*, TYPE> :
    TypemapObject<xs::unievent::websocket::Connection::Statistics*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMG, StaticCast>
{
    static std::string package () { return "UniEvent::WebSocket::Connection::Statistics"; }
};

template <> struct Typemap<unievent::websocket::Connection::Statistics>  {
    static Ref out(const unievent::websocket::Connection::Statistics& s, const Sv& = Sv()) {
        return Ref::create(Hash {
            {"msgs_in",  xs::out<size_t>(s.msgs_in)},
            {"msgs_out", xs::out<size_t>(s.msgs_out)},
            {"bytes_in",  xs::out<size_t>(s.bytes_in)},
            {"bytes_out", xs::out<size_t>(s.bytes_out)}
        });
    }
};

template <class TYPE> struct Typemap<xs::unievent::websocket::ClientConnectRequest*, TYPE> :
    Typemap<xs::protocol::websocket::ConnectRequest*, TYPE>
{
    static std::string package () { return "UniEvent::WebSocket::ConnectRequest"; }
};

template <class TYPE>
struct Typemap<xs::unievent::websocket::ClientConnectRequestSP, panda::iptr<TYPE>> : Typemap<TYPE*> {
    using Super = Typemap<TYPE*>;
    static panda::iptr<TYPE> in (Sv arg) {
        if (!arg.defined()) return {};
        if (arg.is_object_ref()) return Super::in(arg);
        panda::iptr<TYPE> ret = make_backref<TYPE>();
        xs::unievent::websocket::make_request(arg, ret.get());
        return ret;
    }
};

template <> struct Typemap<panda::unievent::websocket::Location> : TypemapBase<panda::unievent::websocket::Location> {
    using Location = panda::unievent::websocket::Location;
    static Location in (SV* arg) {
        const Hash h = arg;
        Scalar val;
        Location loc;
        if ((val = h["host"]))       loc.host       = xs::in<panda::string>(val);
        if ((val = h["port"]))       loc.port       = Simple(val);
        if ((val = h["name"]))       loc.name       = xs::in<panda::string>(val);
        if ((val = h["ssl_ctx"]))    loc.ssl_ctx    = xs::in<panda::unievent::SslContext>(val);
        if ((val = h["backlog"]))    loc.backlog    = Simple(val);
        if ((val = h["reuse_port"])) loc.reuse_port = val.is_true();
        return loc;
    }

    static Ref out(Location loc, const xs::Sv& = Sv()) {
        return Ref::create(Hash {
            {"host",  xs::out(loc.host)},
            {"port",  xs::out(loc.port)},
            {"name",  xs::out(loc.name)},
            {"ssl_ctx",  xs::out(loc.ssl_ctx)},
            {"backlog",  xs::out(loc.backlog)},
            {"reuse_port",  xs::out(loc.reuse_port)},
        });
    }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Connection::Config, TYPE> : Typemap<panda::protocol::websocket::Parser::Config, TYPE> {
    using Super = Typemap<panda::protocol::websocket::Parser::Config, TYPE>;
    static TYPE in (SV* arg) {
        const Hash h = arg;
        TYPE cfg = Super::in(arg);
        Scalar val;
        if ((val = h.fetch("tcp_nodelay")))      cfg.tcp_nodelay = val.is_true();
        if ((val = h.fetch("shutdown_timeout"))) cfg.shutdown_timeout = xs::unievent::websocket::get_time(Simple(val));
        return cfg;
    }

    static Sv out (TYPE var, const Sv& = Sv()) {
        Ref ref = Super::out(var);
        Hash h = ref.value<Hash>();
        h.store("tcp_nodelay", xs::out(var.tcp_nodelay));
        h.store("shutdown_timeout", xs::out(var.shutdown_timeout));
        return Sv(ref);
    }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::ServerConnection::Config, TYPE> : Typemap<panda::unievent::websocket::Connection::Config, TYPE> {
    using Super = Typemap<panda::unievent::websocket::Connection::Config, TYPE>;
    static TYPE in (SV* arg) {
        const Hash h = arg;
        TYPE cfg = Super::in(arg);
        Scalar val;
        if ((val = h.fetch("connect_timeout"))) cfg.connect_timeout = xs::unievent::websocket::get_time(Simple(val));
        return cfg;
    }

    static Sv out (TYPE var, const Sv& = Sv()) {
        Ref ref = Super::out(var);
        Hash h = ref.value<Hash>();
        h.store("connect_timeout", xs::out(var.connect_timeout));
        return Sv(ref);
    }
};

template <class TYPE> struct Typemap<panda::unievent::websocket::Server::Config, TYPE> : TypemapBase<panda::unievent::websocket::Server::Config, TYPE> {
    using Location = panda::unievent::websocket::Location;
    static TYPE in (SV* arg) {
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

    static Sv out (TYPE var, const Sv& = Sv()) {
        Hash ret = Hash::create();
        ret.store("locations", xs::out(var.locations));
        ret.store("connection", xs::out(var.connection));
        return Ref::create(ret);
    }
};

}
