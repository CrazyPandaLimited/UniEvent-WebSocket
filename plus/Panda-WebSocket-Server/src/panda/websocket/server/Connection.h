#pragma once
#include <panda/websocket/BaseConnection.h>
#include <panda/websocket/ServerParser.h>
#include <panda/CallbackDispatcher.h>

namespace panda { namespace websocket { namespace server {

using panda::event::TCP;
using panda::event::Loop;
using panda::event::StreamError;
using panda::CallbackDispatcher;

class Server;

struct Connection : virtual BaseConnection {
    CallbackDispatcher<void(iptr<Connection>, ConnectRequestSP)> accept_callback;

    Connection (Server* server, uint64_t id);
    
    uint64_t id () const { return _id; }
    
    virtual void run (Stream* listener);
    
    virtual void send_accept_error    (HTTPResponse* res);
    virtual void send_accept_response (ConnectResponse* res);

    using BaseConnection::close;
    virtual void close(uint16_t code = uint16_t(CloseCode::DONE), string payload = string()) override;

    struct Conf {
        BaseConnection::Conf base;
        size_t max_handshake_size = 0;
    };

    void configure(Conf conf);

protected:
    virtual void on_accept       (ConnectRequestSP request);

    virtual ~Connection () {
        panda_log_debug("connection destroy");
    }

private:
    uint64_t     _id;
    Server*      _server;
    ServerParser _parser;
    bool         _alive;
    
    void on_read (const string& buf, const StreamError& err) override;
};

using ConnectionSP = iptr<Connection>;

std::ostream& operator <<(std::ostream& stream, const Connection::Conf& conf);

}}}
