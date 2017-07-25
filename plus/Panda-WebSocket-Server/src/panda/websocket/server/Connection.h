#pragma once
#include <panda/event/TCP.h>
#include <panda/websocket/ServerParser.h>

namespace panda { namespace websocket { namespace server {

using panda::event::TCP;
using panda::event::Loop;
using panda::event::StreamError;
class Server;

class Connection : public TCP {
public:
    std::function<void(Connection*, ConnectRequestSP)>   accept_callback;
    std::function<void(Connection*, FrameSP)>            frame_callback;
    std::function<void(Connection*, MessageSP)>          message_callback;
    std::function<void(Connection*, const StreamError&)> stream_error_callback;

    Connection (Server* server, uint64_t id);
    
    uint64_t id () const { return _id; }

    template<typename... Args>
    void send_message(Args&&... args) override {
        auto all = _parser.send_message(std::forward<Args>(args)..., Opcode::BINARY);
        write(all.begin(), all.end());
    }

    template<typename... Args>
    void send_text(Args&&... args) override {
        auto all = _parser.send_message(std::forward<Args>(args)..., Opcode::TEXT);
        write(all.begin(), all.end());
    }
    
    virtual void run (Stream* listener);
    
    virtual void send_accept_error    (HTTPResponse* res);
    virtual void send_accept_response (ConnectResponse* res);

    virtual void close (int code);
    void close (CloseCode code) { close((int)code); }

    virtual ~Connection ();

protected:
    virtual void on_accept       (ConnectRequestSP request);
    virtual void on_frame        (FrameSP frame);
    virtual void on_message      (MessageSP msg);
    virtual void on_stream_error (const StreamError& err);
    virtual void on_eof          ();

private:
    uint64_t     _id;
    Server*      _server;
    ServerParser _parser;
    
    void on_read (const string& buf, const StreamError& err) override;

};

typedef shared_ptr<Connection> ConnectionSP;

}}}
