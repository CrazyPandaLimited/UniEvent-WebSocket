#include <xs/xs.h>
#include <xs/websocket/server.h>
#include <xs/event/error.h>
#include <panda/websocket/server.h>
#include <iostream>

using namespace panda::websocket::server;
using namespace panda::websocket;
using namespace xs::websocket::server;
using namespace xs::websocket;
using panda::event::Loop;
using xs::event::error_sv;

using std::endl;

MODULE = Panda::WebSocket::Server                PACKAGE = Panda::WebSocket::Server
PROTOTYPES: DISABLE

INCLUDE: Server.xsi

INCLUDE: Connection.xsi

