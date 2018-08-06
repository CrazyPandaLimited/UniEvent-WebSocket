#include "server.h"

namespace xs { namespace websocket { namespace server {

using panda::string;

Location XSServer::make_location (const Hash& hvloc) {
    Scalar val;
    Location loc = {string(), 0, false, true, 0, nullptr};
    if ((val = hvloc.fetch("host")))       loc.host   = xs::in<string>(val);
    if ((val = hvloc.fetch("port")))       loc.port   = SvUV(val);
    if ((val = hvloc.fetch("secure")))     loc.secure = val.is_true();
    if ((val = hvloc.fetch("ssl_ctx")))    loc.ssl_ctx = xs::in<SSL_CTX*>(val);
    if ((val = hvloc.fetch("backlog")))    loc.backlog = SvUV(val);
    if ((val = hvloc.fetch("reuse_port"))) loc.reuse_port = val.is_true();
    return loc;
}

ServerConfig XSServer::make_server_config (const Hash& hvcfg) {
    Scalar val;
    ServerConfig config;

    auto locs_av = xs::in<Array>(hvcfg.fetch("locations"));
    if (locs_av) {
        for (const auto& elem : locs_av) {
            Hash hvloc = elem;
            if (!hvloc) throw "location should be a hash";
            config.locations.push_back(make_location(hvloc));
        }
    }
    return config;
}

}}}
