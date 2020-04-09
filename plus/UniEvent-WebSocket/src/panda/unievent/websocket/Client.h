#pragma once
#include "Connection.h"
#include "WrapperTimeout.h"
#include <panda/protocol/websocket/ClientParser.h>

namespace panda { namespace unievent { namespace websocket {

struct ClientConnectRequest : panda::protocol::websocket::ConnectRequest {
    using panda::protocol::websocket::ConnectRequest::ConnectRequest;

    WrapperTimeout timeout;

    unievent::AddrInfoHints addr_hints = Tcp::defhints;
    bool cached_resolver = true;
};

using ClientConnectRequestSP = iptr<ClientConnectRequest>;

struct Client;
using ClientSP = iptr<Client>;

struct Client : virtual Connection {
    using connect_fptr = void(const ClientSP&, const ConnectResponseSP&);
    using connect_fn   = function<connect_fptr>;

    static Config default_config;

    CallbackDispatcher<connect_fptr> connect_event;

    Client (const LoopSP& loop = Loop::default_loop(), const Config& = default_config);

    void connect (const ClientConnectRequestSP& request);
    void connect (const string& host_path, bool secure = false, uint16_t port = 0);

protected:
    using Connection::on_connect; // suppress 'hide' warnings

    virtual void on_connect (const ConnectResponseSP& response);

    void on_connect (const ErrorCode&, const unievent::ConnectRequestSP&) override;
    void on_read    (string& buf, const ErrorCode&) override;

    void do_close (uint16_t code, const string&) override;

    using Tcp::connect;

    ClientConnectRequestSP connect_request;
private:
    void call_on_connect(const ConnectResponseSP& response);

    ClientParser parser;
};

inline string ws_scheme(bool secure = false) {
    return secure ? string("wss") : string("ws");
}

}}}
