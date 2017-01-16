#pragma once
#include <map>
#include <vector>
#include <panda/refcnt.h>
#include <panda/websocket/server/inc.h>
#include <panda/websocket/server/error.h>
#include <panda/websocket/server/Listener.h>
#include <panda/websocket/server/Connection.h>

namespace panda { namespace websocket { namespace server {

struct ServerConfig {
    std::vector<Location> locations;
};

class Server : public virtual RefCounted {
public:

    Server (Loop* loop);
    
    void init (ServerConfig config) throw(ConfigError);
    
    Loop* loop () const { return _loop; }
    
    void run  ();
    void stop ();
    
    virtual ~Server ();

private:
    friend class Listener;
    typedef std::map<uint64_t, shared_ptr<Connection>> ConnectionMap;

    shared_ptr<Loop>                  _loop;
    std::vector<Location>             locations;
    std::vector<shared_ptr<Listener>> listeners;
    uint64_t                          lastid;
    bool                              running;
    ConnectionMap                     connections;
    
    void on_connect        (Stream* handle, const StreamError& err);
    void on_disconnect     (Stream* handle);
    void remove_connection (Connection* conn);

};

}}}
