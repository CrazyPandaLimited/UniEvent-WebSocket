#include <xs/xs.h>
#include <xs/websocket/server.h>
#include <xs/event/error.h>
#include <panda/websocket/server.h>
#include <panda/websocket/Client.h>
#include <panda/websocket/BaseConnection.h>
#include <xs/lib/NativeCallbackDispatcher.h>
#include <iostream>

using namespace panda::websocket::server;
using namespace panda::websocket;
using namespace xs::websocket::server;
using namespace xs::websocket;
using panda::event::Loop;
using xs::event::error_sv;
using panda::websocket::Client;
using xs::lib::NativeCallbackDispatcher;
using panda::function;

using std::endl;

MODULE = Panda::WebSocket::Server                PACKAGE = Panda::WebSocket::Server
PROTOTYPES: DISABLE

INCLUDE: Server.xsi

INCLUDE: Connection.xsi

INCLUDE: Client.xsi

