#include "Connection.h"
#include <panda/encode/base16.h>

namespace panda { namespace unievent { namespace websocket {

Builder::Builder (Builder&& b) : MessageBuilder(std::move(b)), _connection{b._connection} {}

Builder::Builder (Connection& connection) : MessageBuilder(connection.parser->message()), _connection{connection} {}

void Builder::send (string& payload, const Stream::write_fn& callback) {
    auto all = MessageBuilder::send(payload);
    _connection.write(all.begin(), all.end(), callback);
}

void Connection::configure (const Config& conf) {
    parser->configure(conf);
}

bool Connection::connected () const { return parser->established() && !parser->send_closed(); }

static void log_use_after_close () {
    panda_log_debug("using websocket::Connection after close");
}

void Connection::on_read (string& buf, const CodeError& err) {
    panda_log_verbose_debug("Websocket " << is_valid() << " on read:" << logger::escaped{buf});
    Tcp::on_read(buf, err);
    if (!is_valid()) return log_use_after_close(); // just ignore everything, we are here after close
    assert(parser->established());
    if (err) return on_error(err);

    auto msg_range = parser->get_messages(buf);
    for (const auto& msg : msg_range) {
        if (msg->error) {
            panda_log_debug("protocol error :" << msg->error);
            return close(CloseCode::PROTOCOL_ERROR);
        }
        switch (msg->opcode()) {
            case Opcode::CLOSE:
                panda_log_debug("connection closed by peer:" << msg->close_code());
                return on_peer_close(msg);
            case Opcode::PING:
                on_ping(msg);
                break;
            case Opcode::PONG:
                on_pong(msg);
                break;
            default:
                on_message(msg);
        }
        if (!parser->established()) break;
    }
}

void Connection::on_frame (const FrameSP& frame) {
    if (Log::should_log(logger::VERBOSE_DEBUG, _panda_code_point_)) {
        Log logger = Log(_panda_code_point_, logger::VERBOSE_DEBUG);
        logger << "websocket Connection::on_frame: payload=\n";
        for (const auto& str : frame->payload) logger << encode::encode_base16(str);
    }
    frame_event(this, frame);
}

void Connection::on_message (const MessageSP& msg) {
    if (Log::should_log(logger::VERBOSE_DEBUG, _panda_code_point_)){
        Log logger = Log(_panda_code_point_, logger::VERBOSE_DEBUG);
        logger << "websocket Connection::on_message: payload=\n";
        for (const auto& str : msg->payload) logger << encode::encode_base16(str);
    }
    message_event(this, msg);
}

void Connection::send_ping (string& payload) {
    if (!is_valid()) return log_use_after_close(); // just ignore everything, we are here after close
    auto all = parser->send_ping(payload);
    write(all.begin(), all.end());
}

void Connection::send_pong (string& payload) {
    if (!is_valid()) return log_use_after_close(); // just ignore everything, we are here after close
    auto all = parser->send_pong(payload);
    write(all.begin(), all.end());
}

void Connection::on_peer_close (const MessageSP& msg) {
    peer_close_event(this, msg);
    close(msg->close_code(), msg->close_message());
}

void Connection::on_ping (const MessageSP& msg) {
    ping_event(this, msg);
    if (msg->payload_length() > Frame::MAX_CONTROL_PAYLOAD) {
        panda_log_info("something weird, ping payload is bigger than possible");
        send_pong(); // send pong without payload
        return;
    }
    switch (msg->payload.size()) {
        case 0: send_pong(); break;
        case 1: send_pong(msg->payload.front()); break;
        default:
            string acc;
            for (const auto& str : msg->payload) acc += str;
            send_pong(acc);
    }
}

void Connection::on_pong (const MessageSP& msg) {
    pong_event(this, msg);
}

void Connection::on_error (const Error& err) {
    panda_log_info("websocket on_error: " << err.whats());
    error_event(this, err);
    close(CloseCode::ABNORMALLY);
}

void Connection::on_eof () {
    panda_log_info("websocket on_eof");
    if (parser->established()) close(CloseCode::ABNORMALLY);
    Tcp::on_eof();
}

void Connection::on_write (const CodeError& err, const WriteRequestSP& req) {
    Tcp::on_write(err, req);
    if (err) on_error(err);
}

void Connection::close (uint16_t code, const string& payload) {
    panda_log_info("Connection[close]: code=" << code << ", payload:" << payload);
    bool established = parser->established();
    if (established && !parser->send_closed()) {
        auto data = parser->send_close(code, payload);
        write(data.begin(), data.end());
    }
    parser->reset();
    close_tcp();
    if (established) on_close(code, payload);
}

void Connection::on_close (uint16_t code, const string& payload) {
    close_event(this, code, payload);
}

void Connection::close_tcp () {
    if (Tcp::connected()) {
        shutdown();
        disconnect();
    } else {
        Tcp::set_connected(false);
        reset();
    }
    valid = false;
}

std::ostream& operator<< (std::ostream& stream, const Connection::Config& conf) {
    stream << "Connection::Config{max_frame_size:" << conf.max_frame_size
           << ", max_message_size:" << conf.max_message_size
           << ", max_handshake_size:" << conf.max_handshake_size
           << "}";
    return stream;
}

}}}
