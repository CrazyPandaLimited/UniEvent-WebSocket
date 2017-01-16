#pragma once
#include <panda/websocket/server/inc.h>

namespace panda { namespace websocket { namespace server {

struct Location {
    string   host;
    uint16_t port;
    bool     secure;
    int      backlog;
};

class Listener : public TCP {
public:
    Listener (Loop* loop);

    void run (Location location);
    
    Location location () const { return _location; }
    
private:
    Location _location;
};

}}}
