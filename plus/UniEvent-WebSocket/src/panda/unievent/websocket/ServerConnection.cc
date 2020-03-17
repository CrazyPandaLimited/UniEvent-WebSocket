#include "ServerConnection.h"
#include "Server.h"
#include <panda/encode/base16.h>

namespace panda { namespace unievent { namespace websocket {

static const auto& panda_log_module = uewslog;

ServerConnection::ServerConnection (Server* server, uint64_t id, const Config& conf) : Connection(server->loop()), _id(id), server(server) {
    panda_log_notice("ServerConnection[new]: id = " << _id);
    init(parser);
    configure(conf);
    set_nodelay(true);
    _state = State::TCP_CONNECTING;
}

void ServerConnection::run (Listener*) {
    _state = State::CONNECTING;
}

void ServerConnection::on_read (string& _buf, const ErrorCode& err) {
    if (_state == State::INITIAL) { // just ignore everything, we are here after close
        panda_log_info("use websocket::ServerConnection " << id() << " after close");
        return;
    }

    string buf = string(_buf.data(), _buf.length()); // TODO - REMOVE COPYING
    if (_state == State::CONNECTED) return Connection::on_read(buf, err);
    assert(_state == State::CONNECTING);

    if (err) {
        panda_log_notice("Websocket accept error: " << err);
        ConnectRequestSP creq = new protocol::websocket::ConnectRequest();
        creq->error = nest_error(errc::READ_ERROR, err);
        on_accept(creq);
        close();
        return;
    }

    panda_log_debug("Websocket on read (accepting):" << log::escaped{buf});

    assert(!parser.accept_parsed());

    auto creq = parser.accept(buf);
    if (!creq) return;

    if (creq->error) {
        panda_log_notice("Websocket accept error: " << creq->error);
        panda::protocol::http::ResponseSP res = new panda::protocol::http::Response();
        send_accept_error(res);
        on_accept(creq);
        close();
        return;
    }

    if (server->accept_filter) {
        panda::protocol::http::ResponseSP res = server->accept_filter(creq);
        if (res) {
            send_accept_error(res.get());
            close();
            return;
        }
    }

    _state = State::CONNECTED;
    on_accept(creq);
}

void ServerConnection::on_accept (const ConnectRequestSP& req) {
    if (!req->error) {
        ConnectResponse res;
        send_accept_response(&res);
    }
    accept_event(this, req);
}

void ServerConnection::send_accept_error (panda::protocol::http::Response* res) {
    write(parser.accept_error(res));
}

void ServerConnection::send_accept_response (ConnectResponse* res) {
    write(parser.accept_response(res));
    auto using_deflate = parser.is_deflate_active();
    panda_log_notice("websocket::ServerConnection " << id() << " has been accepted, "
                    << "deflate is " << (using_deflate ? "on" : "off"));

    panda_elog_debug({
        auto deflate_cfg = parser.effective_deflate_config();
        if (deflate_cfg) {
            log << "websocket::ServerConnection " << id() << " agreed deflate settings"
                << ": server_max_window_bits = " << (int)deflate_cfg->server_max_window_bits
                << ", client_max_window_bits = " << (int)deflate_cfg->client_max_window_bits
                << ", server_no_context_takeover = " << deflate_cfg->server_no_context_takeover
                << ", client_no_context_takeover = " << deflate_cfg->client_no_context_takeover;
        }
    });
}

void ServerConnection::do_close (uint16_t code, const string& payload) {
    Connection::do_close(code, payload);
    if (server) server->remove_connection(this, code, payload); // server might have been removed in callbacks
}

}}}
