#include "XSServer.h"
#include "XSConnection.h"

namespace xs { namespace websocket { namespace server {

Connection* XSServer::new_connection(uint64_t id) {
    return new XSConnection(this, id);
}

}}}
