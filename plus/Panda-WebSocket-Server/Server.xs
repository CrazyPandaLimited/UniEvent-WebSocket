#include <xs/websocket/server.h>
#include <panda/websocket/server.h>
#include <iostream>

using namespace panda::websocket::server;
using namespace xs::websocket::server;
using panda::event::Loop;

using std::cout;

#define PWS_TRY(code)                                           \
    try { code; }                                               \
    catch (const Error& err) { croak_sv(error_sv(err, true)); } \
    catch (const std::exception& err) {                         \
        Error myerr(err.what());                                \
        croak_sv(error_sv(myerr, true));                        \
    }

MODULE = Panda::WebSocket::Server                PACKAGE = Panda::WebSocket::Server
PROTOTYPES: DISABLE

INCLUDE: Server.xsi

MODULE = Panda::WebSocket::Server                PACKAGE = Panda::WebSocket::Server
PROTOTYPES: DISABLE