#pragma once

#include <panda/websocket/server.h>
#include <xs/xs.h>

namespace xs { namespace websocket { namespace server {

using panda::websocket::server::Server;
using panda::websocket::server::Connection;

class XSServer : public Server, public xs::XSBackref {
public:
    SvIntrPtr connection_class;

    using Server::Server;

    Connection* new_connection (uint64_t id) override;
};

}}}
