#include "Connection.h"
#include "../Server.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

using std::endl;
using namespace std::placeholders;

namespace panda { namespace unievent { namespace websocket { namespace server {

Connection::Connection (Server* server, uint64_t id, const Config& conf) : ConnectionBase(server->loop()), _id(id), server(server) {
    panda_log_info("Connection[new]: id = " << _id);
    init(parser);
    configure(conf);
}

void Connection::run (Stream* listener) {
    listener->accept(this); // TODO: concurrent non-blocking accept in multi-thread may result in not accepting (err from libuv?)
    read_start();
}

void Connection::on_read (string& buf, const CodeError* err) {
    if (parser.established()) return ConnectionBase::on_read(buf, err);
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

void Connection::on_accept (ConnectRequestSP req) {
    ConnectResponse res;
    send_accept_response(&res);
    accept_event(this, req);
}

void Connection::send_accept_error (HTTPResponse* res) {
    write(parser.accept_error(res));
}

void Connection::send_accept_response (ConnectResponse* res) {
    write(parser.accept_response(res));
}

void Connection::close (uint16_t code, const string& payload) {
    ConnectionSP sp = this; // keep self from destruction if user loses all references, that how panda::unievent::TCP works
    bool call = TCP::connecting() || TCP::connected();
    ConnectionBase::close(code, payload);
    if (call) server->remove_connection(sp, code, payload);
}

}}}}
