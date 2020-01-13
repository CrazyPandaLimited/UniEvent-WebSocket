#pragma once
#include "Connection.h"
#include <panda/protocol/websocket/ClientParser.h>

namespace panda { namespace unievent { namespace websocket {

using panda::protocol::websocket::ConnectRequestSP;

struct Client;
using ClientSP = iptr<Client>;

struct Client : virtual Connection {
    static Config default_config;

    Client (const LoopSP& loop = Loop::default_loop(), const Config& = default_config);

    CallbackDispatcher<void(const ClientSP&, const ConnectResponseSP&)> connect_event;

    /** @param port default value is 443 for secure and 80 for usual     */
    void connect (const ConnectRequestSP& request);


protected:
    virtual void on_connect (const ConnectResponseSP& response);

    void do_close (uint16_t code, const string&) override;

    using Tcp::connect;

private:
    void on_connect (const CodeError&, const unievent::ConnectRequestSP&) override;
    void on_read    (string& buf, const CodeError&) override;

    ClientParser parser;
};

inline string ws_scheme(bool secure = false) {
    return secure ? string("wss") : string("ws");
}

}}}
