#include "Client.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

namespace panda  { namespace websocket {

Client::Client(event::Loop* loop)
    : BaseConnection(loop)
{
    init(parser);
}

Client::~Client()
{}

void Client::connect(ConnectRequestSP request, bool secure, uint16_t port) {
    parser.reset();
    if (!port) {
        port = secure ? 443 : 80;
    }
    string port_str = string::from_number(port);
    panda_log_debug("connecting to " << request->uri->host() << ":" << port_str);
    state = State::CONNECTING;
    if (secure) {
        use_ssl();
    }
    connect(request->uri->host(), port_str);
    read_start();
    write(parser.connect_request(request));
}

void Client::close(uint16_t code, string payload) {
    if (state == State::CONNECTING) {
        on_stream_error(StreamError(panda::event::ERRNO_ECANCELED));
    } else {
        BaseConnection::close(code, payload);
    }
}

void Client::on_stream_error(const event::StreamError& err) {
    if (state == State::CONNECTING) {
        state = State::DISCONNECTED;
        ConnectResponseSP res = new ConnectResponse();
        res->error = err.what();
        on_connect(res);
    }
    BaseConnection::on_stream_error(err);
}

void Client::on_connect(ConnectResponseSP response) {
    connect_callback(this, response);
}

void Client::on_connect(const event::StreamError& err, event::ConnectRequest* req) {
    if (err) {
        on_stream_error(err);
        return;
    }
    state = State::TCP_CONNECTED;
}

void Client::on_read(const string& buf, const event::StreamError& err) {
    if (err) {
        on_stream_error(err);
        return;
    }

    // parser breaks const-ness, so force a copy here
    string chunk = string(buf.data(), buf.length());

    if (state == State::TCP_CONNECTED) {
        auto req = parser.connect(chunk);
        if (!req) return;

        if (req->error) {
            on_connect(req);
        }
        else {
            state = State::WS_CONNECTED;
            on_connect(req);
        }
        chunk.clear();
    }
    if (state == State::WS_CONNECTED) {
        auto msg_range = parser.get_messages(chunk);
        for (const auto& msg : msg_range) {
            if (msg->error) return close(CloseCode::PROTOCOL_ERROR);
            if (msg->opcode() == Opcode::CLOSE) {
                panda_log_debug("connection closed by server:" << msg->close_code());
                return close(msg->close_code());
            }
            if (msg->opcode() == Opcode::PING) write(parser.send_pong());
            on_message(msg);
            if (state != State::WS_CONNECTED) {
                break;
            }
        }
    }
}

}}
