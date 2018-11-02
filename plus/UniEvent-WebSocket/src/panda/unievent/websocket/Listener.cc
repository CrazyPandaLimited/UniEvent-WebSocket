#include "Listener.h"
#include <panda/log.h>

using namespace panda::unievent::websocket;

// initializing TCP with two params ctor: this way it will pre-create socket so we could set its options in run()
Listener::Listener (Loop* loop, const Location& loc) : TCP(loop, (unsigned)AF_INET), _location(loc) {
    if (_location.ssl_ctx) _location.secure = true;
}

void Listener::run () {
    panda_log_info("Listener[run]: listening " << (_location.secure ? "wss://" : "ws://") << _location.host << ":" << _location.port
                   << ", backlog:" << _location.backlog);
    if (_location.reuse_port) {
        int on = 1;
        setsockopt(SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    }
    bind(_location.host, _location.port);
    listen(_location.backlog);
    if (_location.ssl_ctx) use_ssl(_location.ssl_ctx);
}