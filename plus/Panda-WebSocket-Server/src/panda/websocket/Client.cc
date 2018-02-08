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
    panda_debug_v(request->uri->host());
    panda_debug_v(port_str);
    connect(request->uri->host(), port_str);
    read_start();
    write(parser.connect_request(request));
}

void Client::on_connect(ConnectResponseSP response) {
    panda_log_info("on_connect");
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
    panda_log_info("on_read: " << encode::encode_base16(buf));
    if (err) {
        on_stream_error(err);
        return;
    }

    string chunk = buf;
    if (state == State::TCP_CONNECTED) {
        panda_log_info("handshake is in proccess");
        auto req = parser.connect(chunk);
        if (!req) return;

        if (req->error) {
            on_any_error(req->error);
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
            if (msg->opcode() == Opcode::CLOSE) return close(msg->close_code());
            if (msg->opcode() == Opcode::PING) write(parser.send_pong());
            on_message(msg);
            if (state != State::WS_CONNECTED) {
                break;
            }
        }
    }
}

}}
