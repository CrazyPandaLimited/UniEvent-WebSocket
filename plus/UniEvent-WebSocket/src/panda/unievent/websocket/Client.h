#pragma once
#include "Connection.h"
#include <panda/CallbackDispatcher.h>
#include <panda/protocol/websocket/ClientParser.h>

namespace panda { namespace unievent { namespace websocket {

using namespace panda::protocol::websocket;
using panda::protocol::websocket::ConnectRequestSP;

struct Client : virtual Connection {
    using ClientSP = iptr<Client>;

    Client (Loop* loop = Loop::default_loop());

    CallbackDispatcher<void(ClientSP, ConnectResponseSP)>  connect_callback;

    /** @param port default value is 443 for secure and 80 for usual     */
    void connect (ConnectRequestSP request, bool secure = false, uint16_t port = 0);

    virtual void close (uint16_t code = uint16_t(CloseCode::DONE), string = string()) override;

protected:
    virtual void on_error   (const Error& err) override;
    virtual void on_connect (ConnectResponseSP response);

    using TCP::connect;

    virtual ~Client() {}

private:
    virtual void on_connect (const CodeError* err, ConnectRequest* req) override;
    virtual void on_read    (const string& buf, const CodeError* err) override;

    ClientParser parser;
};

using ClientSP = Client::ClientSP;

}}}
