#pragma once
#include <panda/CallbackDispatcher.h>
#include <panda/unievent/websocket/Connection.h>
#include <panda/protocol/websocket/ServerParser.h>

namespace panda { namespace unievent { namespace websocket {

using panda::CallbackDispatcher;
using panda::protocol::websocket::ConnectRequestSP;

struct Server;

struct ServerConnection : virtual Connection {
    using ServerConnectionSP = iptr<ServerConnection>;

    CallbackDispatcher<void(ServerConnectionSP, ConnectRequestSP)> accept_callback;

    ServerConnection (Server* server, uint64_t id);
    
    uint64_t id () const { return _id; }
    
    virtual void run (Stream* listener);
    
    virtual void send_accept_error    (HTTPResponse* res);
    virtual void send_accept_response (ConnectResponse* res);

    using Connection::close;
    virtual void close (uint16_t code = uint16_t(CloseCode::DONE), string payload = string()) override;

    struct Conf {
        Connection::Conf base;
        size_t max_handshake_size = 0;
    };

    void configure (Conf conf);

protected:
    virtual void on_accept (ConnectRequestSP request);

    virtual ~ServerConnection () {
        panda_log_debug("connection destroy");
    }

private:
    uint64_t     _id;
    Server*      _server;
    ServerParser _parser;
    bool         _alive;
    
    void on_read (const string& buf, const StreamError& err) override;
};

using ServerConnectionSP = ServerConnection::ServerConnectionSP;

std::ostream& operator<< (std::ostream& stream, const ServerConnection::Conf& conf);

}}}
