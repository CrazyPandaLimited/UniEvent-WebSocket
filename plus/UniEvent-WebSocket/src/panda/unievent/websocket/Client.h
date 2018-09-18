#pragma once
#include "ConnectionBase.h"
#include <panda/CallbackDispatcher.h>
#include <panda/protocol/websocket/ClientParser.h>

namespace panda { namespace unievent { namespace websocket {

using namespace panda::protocol::websocket;
using panda::protocol::websocket::ConnectRequestSP;

struct Client : virtual ConnectionBase {
    using SP = iptr<Client>;
    static Config default_config;

    Client (Loop* loop = Loop::default_loop(), const Config& = default_config);

    CallbackDispatcher<void(SP, ConnectResponseSP)> connect_event;

    /** @param port default value is 443 for secure and 80 for usual     */
    void connect (ConnectRequestSP request, bool secure = false, uint16_t port = 0);

    virtual void close (uint16_t code = uint16_t(CloseCode::DONE), const string& = string()) override;

protected:
    virtual void on_connect (ConnectResponseSP response);

    using TCP::connect;

    virtual ~Client () {}

private:
    virtual void on_connect (const CodeError* err, ConnectRequest* req) override;
    virtual void on_read    (string& buf, const CodeError* err) override;

    ClientParser parser;
};

using ClientSP = Client::SP;

}}}
