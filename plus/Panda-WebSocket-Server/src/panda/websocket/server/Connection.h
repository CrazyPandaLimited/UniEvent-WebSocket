#pragma once
#include <panda/websocket/server/inc.h>

namespace panda { namespace websocket { namespace server {

class Connection : public TCP {
public:

    Connection (Loop* loop, uint64_t id);
    
    uint64_t id () const { return _id; }
    
    void run (Stream* listener);
    
    virtual ~Connection ();

private:

    uint64_t _id;
    
    virtual bool on_read (const buf_t* buf, const StreamError& err);

};

}}}
