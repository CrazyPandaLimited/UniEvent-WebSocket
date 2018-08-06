#include <xs/xs.h>
#include <xs/websocket/server.h>
#include <xs/lib/NativeCallbackDispatcher.h>
#include <iostream>

using namespace xs;
using namespace xs::websocket;
using namespace panda::websocket;
using namespace xs::websocket::server;
using namespace panda::websocket::server;
using std::string_view;
using panda::event::Loop;
using xs::event::error_sv;
using xs::lib::NativeCallbackDispatcher;
using panda::function;

using std::endl;

MODULE = Panda::WebSocket::Server                PACKAGE = Panda::WebSocket::Server
PROTOTYPES: DISABLE

BOOT {
    Stash main_stash("Panda::WebSocket::Server");
}

INCLUDE: Server.xsi

INCLUDE: BaseConnection.xsi

INCLUDE: Connection.xsi

INCLUDE: Client.xsi

