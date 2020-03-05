#pragma once
#include "Connection.h"
#include <panda/protocol/websocket/ClientParser.h>

namespace panda { namespace unievent { namespace websocket {

struct ClientConnectRequest : panda::protocol::websocket::ConnectRequest {
    using panda::protocol::websocket::ConnectRequest::ConnectRequest;

    unievent::AddrInfoHints addr_hints = Tcp::defhints;
    bool cached_resolver = true;
};

using ClientConnectRequestSP = iptr<ClientConnectRequest>;

struct Client;
using ClientSP = iptr<Client>;

struct Client : virtual Connection {
    static Config default_config;

    Client (const LoopSP& loop = Loop::default_loop(), const Config& = default_config);

    CallbackDispatcher<void(const ClientSP&, const ConnectResponseSP&)> connect_event;

    void connect (const ClientConnectRequestSP& request);


protected:
    using Connection::on_connect; // suppress 'hide' warnings

    virtual void on_connect (const ConnectResponseSP& response);

    void on_connect (const ErrorCode&, const unievent::ConnectRequestSP&) override;
    void on_read    (string& buf, const ErrorCode&) override;

    void do_close (uint16_t code, const string&) override;

    using Tcp::connect;

private:
    ClientParser parser;
};

inline string ws_scheme(bool secure = false) {
    return secure ? string("wss") : string("ws");
}

}}}
