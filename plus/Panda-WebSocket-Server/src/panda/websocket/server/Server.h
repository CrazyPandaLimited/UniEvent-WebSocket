#pragma once
#include <map>
#include <vector>
#include <panda/refcnt.h>
#include <panda/websocket/server/Listener.h>
#include <panda/websocket/server/Connection.h>
#include <panda/CallbackDispatcher.h>

namespace panda { namespace websocket { namespace server {

using panda::event::Stream;

struct ServerConfig {
    std::vector<Location> locations;
};

class Server : public virtual RefCounted {
public:

    Server (Loop* loop = Loop::default_loop());

    void init (ServerConfig config);
    
    Loop* loop () const { return _loop; }
    
    void run  ();
    void stop ();

    void close_connection  (Connection* conn, CloseCode code) { conn->close(code); }
    void close_connection  (Connection* conn, int code)       { conn->close(code); }
    void remove_connection (Connection* conn);

    virtual ~Server ();

    CallbackDispatcher<void (Server*, Connection*)> connection_callback;
    CallbackDispatcher<void (Server*, Connection*)> remove_connection_callback;

protected:
    virtual Connection* new_connection (uint64_t id);
    virtual void on_connection(Connection* conn);
    virtual void on_remove_connection(Connection* conn);

private:
    typedef std::map<uint64_t, ConnectionSP> ConnectionMap;

    shared_ptr<Loop>        _loop;
    std::vector<Location>   locations;
    std::vector<ListenerSP> listeners;
    uint64_t                lastid;
    bool                    running;
    ConnectionMap           connections;
    
    void on_connect        (Stream* handle, const StreamError& err);
    void on_disconnect     (Stream* handle);

};

}}}
