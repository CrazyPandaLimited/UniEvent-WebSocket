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
    
    virtual void run  ();
    virtual void stop ();

    using ConnectionSP = shared_ptr<Connection>;
    void close_connection  (ConnectionSP conn, CloseCode code) { conn->close(code); }
    void close_connection  (ConnectionSP conn, int code)       { conn->close(code); }
    void remove_connection (ConnectionSP conn);

    virtual ~Server ();

    CallbackDispatcher<void (shared_ptr<Server, true>, ConnectionSP)> connection_callback;
    CallbackDispatcher<void (shared_ptr<Server, true>, ConnectionSP)> remove_connection_callback;

protected:
    virtual ConnectionSP new_connection (uint64_t id);
    virtual void on_connection(ConnectionSP conn);
    virtual void on_remove_connection(ConnectionSP conn);

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
