#include "XSServer.h"
#include "XSConnection.h"

namespace xs { namespace websocket { namespace server {

using xs::my_perl;
using panda::string;

Server::ConnectionSP XSServer::new_connection(uint64_t id) {
    return new XSConnection(this, id);
}

Location XSServer::make_location(HV* hvloc) {
    SV** ref;
    Location loc = {string(), 0, false, true, 0, nullptr};
    if ((ref = hv_fetch(hvloc, "host", 4, 0)))        loc.host   = sv2string(*ref);
    if ((ref = hv_fetch(hvloc, "port", 4, 0)))        loc.port   = SvUV(*ref);
    if ((ref = hv_fetch(hvloc, "secure", 6, 0)))      loc.secure = SvTRUE(*ref) ? true : false;
    if ((ref = hv_fetch(hvloc, "ssl_ctx", 7, 0)))     loc.ssl_ctx = (SSL_CTX*)SvIV(*ref);
    if ((ref = hv_fetch(hvloc, "backlog", 7, 0)))     loc.backlog = SvUV(*ref);
    if ((ref = hv_fetch(hvloc, "reuse_port", 10, 0))) loc.reuse_port = SvTRUE(*ref) ? true : false;
    return loc;
}

ServerConfig XSServer::make_server_config(HV* hvcfg) {
    SV** ref;
    ServerConfig config;

    if ((ref = hv_fetch(hvcfg, "locations", 9, 0))) {
        if (!SvOK(*ref) || !SvROK(*ref) || SvTYPE(SvRV(*ref)) != SVt_PVAV)
            croak("locations should be array ref");
        auto av = (AV*)SvRV(*ref);
        auto lasti = AvFILLp(av);
        auto svlocs = AvARRAY(av);
        while (lasti-- >= 0) {
            if (!*svlocs) continue;
            if (!SvOK(*svlocs) || !SvROK(*svlocs) || SvTYPE(SvRV(*svlocs)) != SVt_PVHV)
                croak("location should be hash");
            auto hvloc = (HV*)SvRV(*svlocs);
            config.locations.push_back(make_location(hvloc));
            svlocs++;
        }
    }
    return config;
}

}}}
