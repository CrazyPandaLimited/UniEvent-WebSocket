#include <panda/websocket/server/Server.h>
#include <panda/websocket/server/Listener.h>

namespace panda { namespace websocket { namespace server {

using std::cout;

Listener::Listener (Loop* loop) : TCP(loop, AF_INET) {
}

void Listener::run (Location location) {
    _location = location;
    int on = 1;
    setsockopt(SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    const char* service = panda::lib::itoa(_location.port);
    cout << "Listener[run]: listening " << (_location.secure ? "wss://" : "ws://") << _location.host << ":" << service << "\n";
    bind(_location.host.data(), service);
    listen(_location.backlog);
    if (_location.secure) use_ssl();
}

}}}
