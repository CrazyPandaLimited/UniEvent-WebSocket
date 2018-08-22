#pragma once
#include <map>
#include <vector>
#include <atomic>
#include <panda/refcnt.h>
#include <panda/CallbackDispatcher.h>
#include <panda/unievent/websocket/Listener.h>
#include <panda/unievent/websocket/ServerConnection.h>

namespace panda { namespace unievent { namespace websocket {

struct ServerConfig {
    ServerConfig (const std::vector<Location>& locations_ = {}, const ServerConnection::Conf& conn_conf_ = {}) : locations(locations_), conn_conf(conn_conf_) {}

    std::vector<Location>  locations;
    ServerConnection::Conf conn_conf;
};

struct Server : virtual Refcnt {
    using ServerSP = iptr<Server>;

    Server (Loop* loop = Loop::default_loop());

    void init        (ServerConfig config);
    void reconfigure (const ServerConfig& conf);
    
    Loop* loop () const { return _loop; }
    
    virtual void run  ();
    virtual void stop ();

    void start_listening ();
    void stop_listening  ();

    void close_connection  (ServerConnectionSP conn, uint16_t code)  { conn->close(code); }
    void close_connection  (ServerConnectionSP conn, int code)       { conn->close(code); }
    void remove_connection (ServerConnectionSP conn, uint16_t code = uint16_t(CloseCode::ABNORMALLY), string payload = "");

    virtual ~Server ();

    CallbackDispatcher<void (ServerSP, ServerConnectionSP)>                   connection_callback;
    CallbackDispatcher<void (ServerSP, ServerConnectionSP, uint16_t, string)> disconnection_callback;

protected:
    virtual ServerConnectionSP new_connection (uint64_t id);

    virtual void on_connection        (ServerConnectionSP conn);
    virtual void on_remove_connection (ServerConnectionSP conn, uint16_t code = uint16_t(CloseCode::ABNORMALLY), string payload = "");

    template <class This, typename Config>
    void reconfigure (This* self, const Config& conf) {
        if (!self->running) throw std::logic_error("server must be running for reconfigure");
        self->stop_listening();
        self->init(conf);
        self->start_listening();
    }

    template <class Conn = ServerConnection>
    iptr<Conn> get_connection (uint64_t id) {
        auto iter = connections.find(id);
        if (iter == connections.end()) return nullptr;
        else return dynamic_pointer_cast<Conn>(iter->second);
    }

    bool                                   running;
    std::map<uint64_t, ServerConnectionSP> connections;

private:
    static std::atomic<uint64_t> lastid;

    LoopSP                  _loop;
    std::vector<Location>   locations;
    ServerConnection::Conf  conn_conf;
    std::vector<ListenerSP> listeners;

    void on_connect    (Stream* handle, const StreamError& err);
    void on_disconnect (Stream* handle);
};

using ServerSP = Server::ServerSP;

template <typename Stream>
Stream& operator<< (Stream& stream, const ServerConfig& conf) {
    stream << "ServerConfig{ locations:[";
    for (auto loc : conf.locations) stream << loc << ",";
    stream << "]};";
    return stream;
}

}}}
