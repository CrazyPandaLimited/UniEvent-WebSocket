#pragma once

#include <panda/websocket/server.h>
#include <panda/refcnt.h>
#include <xs/xs.h>

namespace xs { namespace websocket { namespace server {

using panda::websocket::server::Server;
using panda::websocket::server::ServerConfig;
using panda::websocket::server::Location;
using panda::websocket::server::Connection;

class XSServer : public Server, public xs::XSBackref {
public:
    using Server::Server;

    panda::iptr<Connection> new_connection(uint64_t id) override;

    static Location make_location(HV* hvloc);
    static ServerConfig make_server_config(HV* hvcfg);
};

}}}
