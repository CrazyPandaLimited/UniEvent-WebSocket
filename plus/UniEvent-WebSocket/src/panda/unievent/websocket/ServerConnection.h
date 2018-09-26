#pragma once
#include "Connection.h"
#include <panda/CallbackDispatcher.h>
#include <panda/protocol/websocket/ServerParser.h>

namespace panda { namespace unievent { namespace websocket {

struct Server;

using panda::CallbackDispatcher;
using panda::protocol::websocket::ConnectRequestSP;

struct ServerConnection : virtual Connection {
    using SP = iptr<ServerConnection>;

    CallbackDispatcher<void(SP, ConnectRequestSP)> accept_event;

    ServerConnection (Server* server, uint64_t id, const Config& conf);

    uint64_t id () const { return _id; }

    virtual void run (Stream* listener);

    virtual void send_accept_error    (HTTPResponse* res);
    virtual void send_accept_response (ConnectResponse* res);

    virtual void close (uint16_t code = uint16_t(CloseCode::DONE), const string& payload = string()) override;

protected:
    virtual void on_accept (ConnectRequestSP request);

    virtual ~ServerConnection () {
        panda_log_debug("connection destroy");
    }

private:
    uint64_t     _id;
    Server*      server;
    ServerParser parser;

    void on_read (string& buf, const CodeError* err) override;
};

using ServerConnectionSP = ServerConnection::SP;

}}}
