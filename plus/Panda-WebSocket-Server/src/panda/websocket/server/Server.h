#pragma once
#include <map>
#include <vector>
#include <panda/refcnt.h>
#include <panda/websocket/server/Listener.h>
#include <panda/websocket/server/Connection.h>

namespace panda { namespace websocket { namespace server {

using panda::event::Stream;

struct ServerConfig {
    std::vector<Location> locations;
};

class Server : public virtual RefCounted {
public:

    Server (Loop* loop);

    void init (ServerConfig config);
    
    Loop* loop () const { return _loop; }
    
    void run  ();
    void stop ();
    
    virtual ~Server ();

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
    void remove_connection (Connection* conn);

};

}}}
