#pragma once
#include <map>
#include <vector>
#include <atomic>
#include <panda/refcnt.h>
#include <panda/CallbackDispatcher.h>
#include "Listener.h"
#include "ServerConnection.h"

namespace panda { namespace unievent { namespace websocket {

struct Server : virtual Refcnt {
    using SP = iptr<Server>;

    struct Config {
        std::vector<Location> locations;
        Connection::Config    connection;
    };

    Server (Loop* loop = Loop::default_loop());

    virtual void configure (const Config&);
    
    Loop* loop () const { return _loop; }
    
    virtual void run  ();
    virtual void stop ();

    void start_listening ();
    void stop_listening  ();

    void close_connection  (ServerConnectionSP conn, uint16_t code)  { conn->close(code); }
    void close_connection  (ServerConnectionSP conn, int code)       { conn->close(code); }
    void remove_connection (ServerConnectionSP conn, uint16_t code = uint16_t(CloseCode::ABNORMALLY), string payload = "");

    virtual ~Server ();

    CallbackDispatcher<void (SP, ServerConnectionSP)>                   connection_event;
    CallbackDispatcher<void (SP, ServerConnectionSP, uint16_t, string)> disconnection_event;

protected:
    virtual void config_validate (const Config&) const;
    virtual void config_apply    (const Config&);

    virtual ServerConnectionSP new_connection (uint64_t id);

    virtual void on_connection        (ServerConnectionSP conn);
    virtual void on_remove_connection (ServerConnectionSP conn, uint16_t code = uint16_t(CloseCode::ABNORMALLY), const string& payload = {});

    template <class Conn = ServerConnection>
    iptr<Conn> get_connection (uint64_t id) {
        auto iter = connections.find(id);
        if (iter == connections.end()) return {};
        else return dynamic_pointer_cast<Conn>(iter->second);
    }

    bool running;
    std::map<uint64_t, ServerConnectionSP> connections;
    Connection::Config                     conn_conf;

private:
    static std::atomic<uint64_t> lastid;

    LoopSP                  _loop;
    std::vector<Location>   locations;
    std::vector<ListenerSP> listeners;

    void on_connect    (Stream* parent, Stream* handle, const CodeError* err);
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
