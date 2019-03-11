#include "Client.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

using namespace panda::unievent::websocket;

Connection::Config Client::default_config;

Client::Client (Loop* loop, const Connection::Config& conf) : Connection(loop) {
    init(parser);
    configure(conf);
}

void Client::connect (ConnectRequestSP request, bool secure, uint16_t port) {
    init(parser);
    parser.reset();

    if (!request || !request->uri) {
        throw std::invalid_argument("ConnectRequest should contains uri");
    }

    if (!port) port = secure ? 443 : 80;
    panda_log_debug("connecting to " << request->uri->host() << ":" << port);
    if (secure) use_ssl();
    connect(request->uri->host(), port);
    read_start();
    write(parser.connect_request(request));
}

void Client::close (uint16_t code, const string& payload) {
    if ((TCP::connecting() || TCP::connected()) && !parser.established()) {
        panda_log_info("Client::close: connect started but not completed");
        TCP::set_connected(false);
        on_connect(CodeError(ERRNO_ECANCELED), nullptr);
    }
    else Connection::close(code, payload);
}

void Client::on_connect (ConnectResponseSP response) {
    connect_event(this, response);
}

void Client::on_connect (const CodeError* err, ConnectRequest* req) {
    panda_log_verbose_debug("websokcet::Client::on_connect(unievent) " <<  (err ? err->what() : "no errors"));
    TCP::on_connect(err, req);
    if (err) {
        ConnectResponseSP res = new ConnectResponse();
        res->error = err->whats();
        on_error(*err);
        on_connect(res);
    }
}

void Client::on_read (string& _buf, const CodeError* err) {
    if (!is_valid()) { // just ignore everything, we are here after close
        panda_log_debug("use websocket::Client after close");
        return;
    }

    string buf = string(_buf.data(), _buf.length());
    if (parser.established()) return Connection::on_read(buf, err);
    if (err) return on_error(*err);

    auto req = parser.connect(buf);
    if (!req) return;
    on_connect(req);

    if (parser.established() && buf.length()) Connection::on_read(buf, err);
}
