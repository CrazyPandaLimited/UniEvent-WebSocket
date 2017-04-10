#pragma once
#include <panda/event/TCP.h>
#include <panda/websocket/ServerParser.h>

namespace panda { namespace websocket { namespace server {

using panda::event::TCP;
using panda::event::Loop;
using panda::event::StreamError;

class Connection : public TCP {
public:
    std::function<void(Connection*, const StreamError&)> error_callback;

    Connection (Loop* loop, uint64_t id);
    
    uint64_t id () const { return _id; }
    
    void run (Stream* listener);
    
    void ws_accept_error    (HTTPResponse* res);
    void ws_accept_response (ConnectResponse* res);

    virtual ~Connection ();

protected:
    virtual void on_ws_accept  (ConnectRequestSP request);
    virtual void on_ws_message (MessageSP msg);

private:
    uint64_t     _id;
    ServerParser _parser;
    
    void on_read (const string& buf, const StreamError& err) override;

};

typedef shared_ptr<Connection> ConnectionSP;

}}}
