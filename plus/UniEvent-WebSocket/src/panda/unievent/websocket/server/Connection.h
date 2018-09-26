#pragma once
#include "../ConnectionBase.h"
#include <panda/CallbackDispatcher.h>
#include <panda/protocol/websocket/ServerParser.h>

namespace panda { namespace unievent { namespace websocket {

struct Server;

namespace server {

using panda::CallbackDispatcher;
using panda::protocol::websocket::ConnectRequestSP;

struct Connection : virtual ConnectionBase {
    using SP = iptr<Connection>;

    CallbackDispatcher<void(SP, ConnectRequestSP)> accept_event;

    Connection (Server* server, uint64_t id, const Config& conf);

    uint64_t id () const { return _id; }

    virtual void run (Stream* listener);

    virtual void send_accept_error    (HTTPResponse* res);
    virtual void send_accept_response (ConnectResponse* res);

    virtual void close (uint16_t code = uint16_t(CloseCode::DONE), const string& payload = string()) override;

protected:
    virtual void on_accept (ConnectRequestSP request);

    virtual ~Connection () {
        panda_log_debug("connection destroy");
    }

private:
    uint64_t     _id;
    Server*      server;
    ServerParser parser;

    void on_read (string& buf, const CodeError* err) override;
};

using ConnectionSP = Connection::SP;

}}}}
