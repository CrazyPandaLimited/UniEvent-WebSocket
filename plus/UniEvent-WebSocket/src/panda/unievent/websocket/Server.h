#pragma once
#include <map>
#include <vector>
#include <atomic>
#include <panda/refcnt.h>
#include <panda/CallbackDispatcher.h>
#include "server/Listener.h"
#include "server/Connection.h"

namespace panda { namespace unievent { namespace websocket {

struct Server : virtual Refcnt {
    using SP           = iptr<Server>;
    using Location     = server::Location;
    using ListenerSP   = server::ListenerSP;
    using Connection   = server::Connection;
    using ConnectionSP = server::ConnectionSP;

    struct Config {
        std::vector<Location> locations;
        Connection::Config    connection;
    };

    Server (Loop* loop = Loop::default_loop());

    void init        (const Config&);
    void reconfigure (const Config&);
    
    Loop* loop () const { return _loop; }
    
    virtual void run  ();
    virtual void stop ();

    void start_listening ();
    void stop_listening  ();

    void close_connection  (ConnectionSP conn, uint16_t code)  { conn->close(code); }
    void close_connection  (ConnectionSP conn, int code)       { conn->close(code); }
    void remove_connection (ConnectionSP conn, uint16_t code = uint16_t(CloseCode::ABNORMALLY), string payload = "");

    virtual ~Server ();

    CallbackDispatcher<void (SP, ConnectionSP)>                   connection_callback;
    CallbackDispatcher<void (SP, ConnectionSP, uint16_t, string)> disconnection_callback;

protected:
    virtual ConnectionSP new_connection (uint64_t id);

    virtual void on_connection        (ConnectionSP conn);
    virtual void on_remove_connection (ConnectionSP conn, uint16_t code = uint16_t(CloseCode::ABNORMALLY), string payload = "");

    template <class This, typename Config>
    void reconfigure (This* self, const Config& conf) {
        if (!self->running) throw std::logic_error("server must be running for reconfigure");
        self->stop_listening();
        self->init(conf);
        self->start_listening();
    }

    template <class Conn = Connection>
    iptr<Conn> get_connection (uint64_t id) {
        auto iter = connections.find(id);
        if (iter == connections.end()) return {};
        else return dynamic_pointer_cast<Conn>(iter->second);
    }

    bool running;
    std::map<uint64_t, ConnectionSP> connections;

private:
    static std::atomic<uint64_t> lastid;

    LoopSP                  _loop;
    std::vector<Location>   locations;
    Connection::Config      conn_conf;
    std::vector<ListenerSP> listeners;

    void on_connect    (Stream* handle, const CodeError* err);
    void on_disconnect (Stream* handle);
};

using ServerSP = Server::SP;

template <typename Stream>
Stream& operator<< (Stream& stream, const Server::Config& conf) {
    stream << "Server::Config{ locations:[";
    for (auto loc : conf.locations) stream << loc << ",";
    stream << "]};";
    return stream;
}

}}}
