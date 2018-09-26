#include "Client.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

using namespace panda::unievent::websocket;

ConnectionBase::Config Client::default_config;

Client::Client (Loop* loop, const ConnectionBase::Config& conf) : ConnectionBase(loop) {
    init(parser);
    configure(conf);
}

void Client::connect (ConnectRequestSP request, bool secure, uint16_t port) {
    parser.reset();
    if (!port) port = secure ? 443 : 80;
    string port_str = string::from_number(port);
    panda_log_debug("connecting to " << request->uri->host() << ":" << port_str);
    if (secure) use_ssl();
    connect(request->uri->host(), port_str);
    read_start();
    write(parser.connect_request(request));
}

void Client::close (uint16_t code, const string& payload) {
    if ((TCP::connecting() || TCP::connected()) && !parser.established()) {
        panda_log_info("Client::close: connect started but not completed");
        TCP::reset();
        on_connect(CodeError(ERRNO_ECANCELED), nullptr);
    }
    else ConnectionBase::close(code, payload);
}

void Client::on_connect (ConnectResponseSP response) {
    connect_event(this, response);
}

void Client::on_connect (const CodeError* err, ConnectRequest*) {
    if (err) {
        ConnectResponseSP res = new ConnectResponse();
        res->error = err->whats();
        on_connect(res);
        return on_error(*err);
    }
}

void Client::on_read (string& buf, const CodeError* err) {
    if (parser.established()) return ConnectionBase::on_read(buf, err);
    if (err) return on_error(*err);

    auto req = parser.connect(buf);
    if (!req) return;
    on_connect(req);

    if (parser.established() && buf.length()) ConnectionBase::on_read(buf, err);
}
