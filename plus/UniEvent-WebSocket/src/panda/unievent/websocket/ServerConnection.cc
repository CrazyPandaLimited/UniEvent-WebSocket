#include "ServerConnection.h"
#include "Server.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

using std::endl;
using namespace std::placeholders;

namespace panda { namespace unievent { namespace websocket {

ServerConnection::ServerConnection (Server* server, uint64_t id, const Config& conf) : Connection(server->loop()), _id(id), server(server) {
    panda_log_info("ServerConnection[new]: id = " << _id);
    init(parser);
    configure(conf);
}

void ServerConnection::run () {
    read_start();
}

void ServerConnection::on_read (string& _buf, const CodeError* err) {
    if (!is_valid()) { // just ignore everything, we are here after close
        panda_log_debug("use websocket::ServerConnection " << id() << " after close");
        return;
    }
    string buf = string(_buf.data(), _buf.length()); // TODO - REMOVE COPYING
    if (parser.established()) return Connection::on_read(buf, err);
    if (err) return on_error(*err);

    assert(!parser.accept_parsed());

    auto creq = parser.accept(buf);
    if (!creq) return;

    if (creq->error) {
        panda_log_info(creq->error);
        HTTPResponse res;
        send_accept_error(&res);
        close();
        return;
    }

    on_accept(creq);
}

void ServerConnection::on_accept (ConnectRequestSP req) {
    ConnectResponse res;
    send_accept_response(&res);
    accept_event(this, req);
}

void ServerConnection::send_accept_error (HTTPResponse* res) {
    write(parser.accept_error(res));
}

void ServerConnection::send_accept_response (ConnectResponse* res) {
    write(parser.accept_response(res));
}

void ServerConnection::close (uint16_t code, const string& payload) {
    ServerConnectionSP sp = this; // keep self from destruction if user loses all references, that how panda::unievent::TCP works
    bool call = TCP::connecting() || TCP::connected();
    Connection::close(code, payload);
    if (call) server->remove_connection(sp, code, payload);
}

}}}