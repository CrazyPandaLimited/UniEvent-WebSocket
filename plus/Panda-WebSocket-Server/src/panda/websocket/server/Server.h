#pragma once
#include <map>
#include <vector>
#include <panda/refcnt.h>
#include <panda/websocket/server/Listener.h>
#include <panda/websocket/server/Connection.h>
#include <panda/CallbackDispatcher.h>
#include <atomic>

namespace panda { namespace websocket { namespace server {

using panda::event::Stream;

struct ServerConfig {
    std::vector<Location> locations;
    Connection::Conf conn_conf;
};

class Server : public virtual RefCounted {
public:

    Server (Loop* loop = Loop::default_loop());

    void init (ServerConfig config);
    void reconfigure (const ServerConfig& conf);
    
    Loop* loop () const { return _loop; }
    
    virtual void run  ();
    virtual void stop ();

    using ConnectionSP = shared_ptr<Connection>;
    void close_connection  (ConnectionSP conn, uint16_t code) { conn->close(code); }
    void close_connection  (ConnectionSP conn, int code)       { conn->close(code); }
    void remove_connection (ConnectionSP conn, uint16_t code = uint16_t(CloseCode::ABNORMALLY), string payload = "");

    virtual ~Server ();

    CallbackDispatcher<void (shared_ptr<Server, true>, ConnectionSP)> connection_callback;
    CallbackDispatcher<void (shared_ptr<Server, true>, ConnectionSP, uint16_t, string)> disconnection_callback;

protected:
    virtual ConnectionSP new_connection (uint64_t id);
    virtual void on_connection(ConnectionSP conn);
    virtual void on_remove_connection(ConnectionSP conn, uint16_t code = uint16_t(CloseCode::ABNORMALLY), string payload = "");

    void start_listening();
    void stop_listening ();

    template <class This, typename Config>
    void reconfigure(This* self, const Config& conf) {
        if (!self->running) {
            throw std::logic_error("server must be running for reconfigure");
        }

        self->stop_listening();
        self->init(conf);
        self->start_listening();
    }

    template <class Conn = Connection>
    shared_ptr<Conn> get_connection(uint64_t id) {
        auto iter = connections.find(id);
        if (iter == connections.end()) {
            return nullptr;
        } else {
            return dynamic_pointer_cast<Conn>(iter->second);
        }
    }

    bool                    running;

    typedef std::map<uint64_t, ConnectionSP> ConnectionMap;
    ConnectionMap           connections;
private:
    static std::atomic<uint64_t> lastid;

    shared_ptr<Loop>        _loop;
    std::vector<Location>   locations;
    Connection::Conf        conn_conf;
    std::vector<ListenerSP> listeners;

    void on_connect        (Stream* handle, const StreamError& err);
    void on_disconnect     (Stream* handle);

};

template <typename Stream>
Stream& operator <<(Stream& stream, const panda::websocket::server::ServerConfig& conf) {
    stream << "ServerConfig{ locations:[";
    for (auto loc : conf.locations) {
        stream << loc << ",";
    }
    stream << "]};";
    return stream;
}
}}}
