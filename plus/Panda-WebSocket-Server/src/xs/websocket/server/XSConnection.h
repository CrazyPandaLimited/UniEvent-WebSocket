#pragma once

#include <panda/websocket/server/Connection.h>
#include <xs/xs.h>

namespace xs { namespace websocket { namespace server {

using panda::websocket::server::Server;
using panda::websocket::server::Connection;

class XSConnection : public Connection, public XSBackref
{
public:
    using Connection::Connection;
};

}}}
