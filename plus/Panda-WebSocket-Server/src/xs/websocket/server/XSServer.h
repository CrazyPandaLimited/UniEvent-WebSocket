#pragma once
#include <panda/websocket/server.h>

namespace xs { namespace websocket { namespace server {

using panda::websocket::server::Server;

class XSServer : public Server, public xs::XSBackref {
    using Server::Server;
    
};

}}}
