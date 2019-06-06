#include "Client.h"
#include <panda/encode/base16.h>

namespace panda { namespace unievent { namespace websocket {

Connection::Config Client::default_config;

static ConnectResponseSP cres_from_cerr (const CodeError& err) {
    ConnectResponseSP res = new ConnectResponse();
    res->error = err.whats();
    return res;
}

Client::Client (const LoopSP& loop, const Connection::Config& conf) : Connection(loop) {
    init(parser);
    configure(conf);
}

void Client::connect (const ConnectRequestSP& request, bool secure, uint16_t port) {
    if (_state != State::INITIAL) throw std::logic_error("can't connect(): you should close() the client connection first");
    parser.reset();

    if (!request || !request->uri) throw std::invalid_argument("ConnectRequest should contains uri");

    if (!port) port = secure ? 443 : 80;
    panda_log_debug("connecting to " << request->uri->host() << ":" << port);
    if (secure) use_ssl();
    connect(request->uri->host(), port);
    write(parser.connect_request(request));
    _state = State::TCP_CONNECTING;
}

void Client::do_close (uint16_t code, const string& payload) {
    bool connecting = _state == State::CONNECTING;
    Connection::do_close(code, payload);
    if (connecting) {
        panda_log_info("Client::close: connect started but not completed");
        on_connect(cres_from_cerr(std::errc::operation_canceled));
    }
}

void Client::on_connect (const ConnectResponseSP& response) {
    connect_event(this, response);
}

void Client::on_connect (const CodeError& err, const unievent::ConnectRequestSP& req) {
    panda_log_verbose_debug("websokcet::Client::on_connect(unievent) " <<  err.what());
    Tcp::on_connect(err, req);
    if (err) {
        on_error(err);
        on_connect(cres_from_cerr(err));
    }
    else _state = State::CONNECTING;
}

void Client::on_read (string& _buf, const CodeError& err) {
    if (_state == State::INITIAL) { // just ignore everything, we are here after close
        panda_log_debug("use websocket::Client after close");
        return;
    }

    string buf = string(_buf.data(), _buf.length()); // TODO: remove copying
    if (_state == State::CONNECTED) return Connection::on_read(buf, err);

    assert(_state == State::CONNECTING);
    if (err) return on_error(err);
    panda_log_verbose_debug("Websocket on read (connecting):" << log::escaped{buf});

    auto res = parser.connect(buf);
    if (!res) return;

    if (parser.established()) _state = State::CONNECTED;
    else {
        assert(res->error);
        on_error(res->error);
        close();
    }

    on_connect(res);

    if (_state == State::CONNECTED && buf.length()) Connection::on_read(buf, err);
}

}}}
