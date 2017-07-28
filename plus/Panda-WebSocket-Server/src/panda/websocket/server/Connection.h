#pragma once
#include <panda/event/TCP.h>
#include <panda/websocket/ServerParser.h>
#include <panda/CallbackDispatcher.h>

namespace panda { namespace websocket { namespace server {

using panda::event::TCP;
using panda::event::Loop;
using panda::event::StreamError;
using panda::CallbackDispatcher;

class Server;

class Connection : public TCP {
public:
    CallbackDispatcher<void(Connection*, ConnectRequestSP)>   accept_callback;
    CallbackDispatcher<void(Connection*, FrameSP)>            frame_callback;
    CallbackDispatcher<void(Connection*, MessageSP)>          message_callback;
    CallbackDispatcher<void(Connection*, const StreamError&)> stream_error_callback;

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
    bool         _alive;
    
    void on_read (const string& buf, const StreamError& err) override;
    void close();
};

typedef shared_ptr<Connection> ConnectionSP;

}}}
