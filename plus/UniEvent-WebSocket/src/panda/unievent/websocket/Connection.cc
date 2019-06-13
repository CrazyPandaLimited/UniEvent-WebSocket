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

static void log_use_after_close () {
    panda_log_debug("using websocket::Connection after close");
}

void Connection::on_read (string& buf, const CodeError& err) {
    panda_log_verbose_debug("Websocket on read:" << log::escaped{buf});
    Tcp::on_read(buf, err);
    assert(_state == State::CONNECTED && parser->established());
    if (err) return process_error(err);

    auto msg_range = parser->get_messages(buf);
    for (const auto& msg : msg_range) {
        if (msg->error) {
            panda_log_debug("protocol error :" << msg->error);
            auto data = parser->send_close(CloseCode::PROTOCOL_ERROR);
            write(data.begin(), data.end());
            return process_error(msg->error);
        }
        switch (msg->opcode()) {
            case Opcode::CLOSE:
                panda_log_debug("connection closed by peer:" << msg->close_code());
                return process_peer_close(msg);
            case Opcode::PING:
                on_ping(msg);
                break;
            case Opcode::PONG:
                on_pong(msg);
                break;
            default:
                on_message(msg);
        }
        if (_state != State::CONNECTED) break;
    }
}

void Connection::on_message (const MessageSP& msg) {
    panda_elog_verbose_debug({
        log << "websocket Connection::on_message: payload=\n";
        for (const auto& str : msg->payload) log << encode::encode_base16(str);
    });
    message_event(this, msg);
}

void Connection::send_ping () {
    if (_state != State::CONNECTED) return log_use_after_close();
    write(parser->send_ping());
}

void Connection::send_ping (string& payload) {
    if (_state != State::CONNECTED) return log_use_after_close();
    auto all = parser->send_ping(payload);
    write(all.begin(), all.end());
}

void Connection::send_pong () {
    if (_state != State::CONNECTED) return log_use_after_close();
    write(parser->send_pong());
}

void Connection::send_pong (string& payload) {
    if (_state != State::CONNECTED) return log_use_after_close();
    auto all = parser->send_pong(payload);
    write(all.begin(), all.end());
}


void Connection::process_peer_close (const MessageSP& msg) {
    if (_state == State::INITIAL) return; // just ignore everything, we are here after close
    _error_state = true;
    on_peer_close(msg);
    if (_error_state) {
        if (msg) close(msg->close_code(), msg->close_message());
        else     close(CloseCode::ABNORMALLY);
    }
}

void Connection::on_peer_close (const MessageSP& msg) {
    peer_close_event(this, msg);
}

void Connection::on_ping (const MessageSP& msg) {
    if (msg->payload_length() > Frame::MAX_CONTROL_PAYLOAD) {
        panda_log_info("something weird, ping payload is bigger than possible");
        send_pong(); // send pong without payload
    } else {
        switch (msg->payload.size()) {
            case 0: send_pong(); break;
            case 1: send_pong(msg->payload.front()); break;
            default:
                string acc;
                for (const auto& str : msg->payload) acc += str;
                send_pong(acc);
        }
    }
    ping_event(this, msg);
}

void Connection::on_pong (const MessageSP& msg) {
    pong_event(this, msg);
}

void Connection::process_error (const Error& err) {
    panda_log_info("websocket error: " << err.whats());
    if (_state == State::INITIAL) return; // just ignore everything, we are here after close
    _error_state = true;
    on_error(err);
    if (_error_state) close(CloseCode::ABNORMALLY);
}

void Connection::on_error (const Error& err) {
    error_event(this, err);
}

void Connection::on_eof () {
    panda_log_info("websocket on_eof");
    Tcp::on_eof();
    process_peer_close(nullptr);
}

void Connection::on_write (const CodeError& err, const WriteRequestSP& req) {
    panda_log_verbose_debug("websocket on_write: " << err.whats());
    Tcp::on_write(err, req);
    if (err && err.code() != std::errc::operation_canceled) process_error(err);
}

void Connection::do_close (uint16_t code, const string& payload) {
    panda_log_info("Connection[close]: code=" << code << ", payload:" << payload);
    bool was_connected = connected();

    if (was_connected && !parser->send_closed()) {
        auto data = parser->send_close(code, payload);
        write(data.begin(), data.end());
    }
    parser->reset();

    _state = State::INITIAL;

    if (Tcp::connected()) {
        shutdown();
        disconnect();
    } else {
        reset();
    }

    _error_state = false;
    if (was_connected) on_close(code, payload);
}

void Connection::on_close (uint16_t code, const string& payload) {
    close_event(this, code, payload);
}

std::ostream& operator<< (std::ostream& stream, const Connection::Config& conf) {
    stream << "Connection::Config{max_frame_size:" << conf.max_frame_size
           << ", max_message_size:" << conf.max_message_size
           << ", max_handshake_size:" << conf.max_handshake_size
           << "}";
    return stream;
}

}}}
