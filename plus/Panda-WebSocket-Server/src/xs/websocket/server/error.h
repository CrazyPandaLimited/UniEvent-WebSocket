#pragma once
#include <xs/xs.h>
#include <panda/websocket/server/error.h>

namespace xs { namespace websocket { namespace server {

using panda::websocket::server::Error;

SV* error_sv (const Error& err, bool with_mess = false);

}}}
