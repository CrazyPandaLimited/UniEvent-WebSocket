#pragma once
#include <map>
#include <vector>
#include <atomic>
#include "Listener.h"
#include "ServerConnection.h"

namespace panda { namespace unievent { namespace websocket {

struct Server;
using ServerSP = iptr<Server>;

struct Server : virtual Refcntd {
    struct Config {
        std::vector<Location> locations;
        Connection::Config    connection;
    };
    using accept_filter_fn   = function<panda::protocol::http::ResponseSP(const ConnectRequestSP&)>;
    using connection_fptr    = void(const ServerSP&, const ServerConnectionSP&);
    using disconnection_fptr = void(const ServerSP&, const ServerConnectionSP&, uint16_t code, const string& payload);
    using Connections        = std::map<uint64_t, ServerConnectionSP>;

    CallbackDispatcher<connection_fptr>    connection_event;
    CallbackDispatcher<disconnection_fptr> disconnection_event;
    accept_filter_fn                       accept_filter;

    Server (const LoopSP& loop = Loop::default_loop());

    virtual void configure (const Config&);
    
    const LoopSP& loop () const { return _loop; }
    
    virtual void run  ();
    virtual void stop ();
    virtual void stop (uint16_t code);

    void start_listening ();
    void stop_listening  ();

    void close_connection  (const ServerConnectionSP& conn, uint16_t code)  { conn->close(code); }
    void close_connection  (const ServerConnectionSP& conn, int code)       { conn->close(code); }

    template <class Conn = ServerConnection>
    iptr<Conn> get_connection (uint64_t id) {
        auto iter = connections.find(id);
        if (iter == connections.end()) return {};
        else return dynamic_pointer_cast<Conn>(iter->second);
    }

    std::vector<ListenerSP>& get_listeners   () { return listeners; }
    const Connections&       get_connections () { return connections; }

protected:
    bool               running;
    Connections        connections;
    Connection::Config conn_conf;

    virtual void config_validate (const Config&) const;
    virtual void config_apply    (const Config&);

    virtual ServerConnectionSP new_connection (uint64_t id);

    virtual void on_connection    (const ServerConnectionSP& conn);
    virtual void on_disconnection (const ServerConnectionSP& conn, uint16_t = uint16_t(CloseCode::ABNORMALLY), const string& = {});

    void on_delete () noexcept override;

private:
    friend ServerConnection;

    static std::atomic<uint64_t> lastid;

    LoopSP                  _loop;
    std::vector<Location>   locations;
    std::vector<ListenerSP> listeners;

    void on_tcp_connection (const StreamSP&, const StreamSP&, const CodeError&);
    void remove_connection (const ServerConnectionSP& conn, uint16_t code = uint16_t(CloseCode::ABNORMALLY), const string& payload = "");
};

template <typename Stream>
Stream& operator<< (Stream& stream, const Server::Config& conf) {
    stream << "Server::Config{ locations:[";
    for (auto loc : conf.locations) stream << loc << ",";
    stream << "]};";
    return stream;
}

}}}
