#include <xs/lib.h>
#include <xs/websocket/server.h>
#include <panda/websocket/server.h>
#include <iostream>

using namespace panda::websocket::server;
using namespace xs::websocket::server;
using panda::event::Loop;

using std::cout;

MODULE = Panda::WebSocket::Server                PACKAGE = Panda::WebSocket::Server
PROTOTYPES: DISABLE

INCLUDE: Server.xsi

MODULE = Panda::WebSocket::Server                PACKAGE = Panda::WebSocket::Server
PROTOTYPES: DISABLE