#include "Connection.h"
#include <panda/encode/base16.h>
#include <numeric>

namespace panda { namespace unievent { namespace websocket {

Builder::Builder (Builder&& b) : MessageBuilder(std::move(b)), _connection{b._connection} {}

Builder::Builder (Connection& connection) : MessageBuilder(connection.parser->message()), _connection{connection} {}

void Builder::send (string& payload, const Stream::write_fn& callback) {
    if (!_connection.connected()) {
        if (callback) callback(&_connection, CodeError(errc::WRITE_ERROR), new unievent::WriteRequest());
        return;
    }
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
    assert(_state == State::CONNECTED && parser->established());
    if (err) return process_error(ErrorCode(errc::READ_ERROR, ErrorCode(err.code())));
    if (stats) {
        stats->bytes_in += buf.size();
        stats->msgs_in++;
    }

    auto msg_range = parser->get_messages(buf);
    for (const auto& msg : msg_range) {
        if (msg->error) {
            panda_log_debug("protocol error :" << msg->error);
            process_error(ErrorCode(errc::READ_ERROR, msg->error), CloseCode::PROTOCOL_ERROR);
            break;
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

void Connection::process_error (const ErrorCode& err, uint16_t code) {
    panda_log_info("websocket error: " << err.message());
    if (_state == State::INITIAL) return; // just ignore everything, we are here after close
    _error_state = true;
    on_error(err);
    if (_error_state) close(code);
}

void Connection::on_error (const ErrorCode& err) {
    error_event(this, err);
}

void Connection::on_eof () {
    panda_log_info("websocket on_eof");
    process_peer_close(nullptr);
}

void Connection::on_write (const CodeError& err, const WriteRequestSP& req) {
    panda_log_verbose_debug("websocket on_write: " << err.whats());
    if (err && err.code() != std::errc::operation_canceled && err.code() != std::errc::broken_pipe && err.code() != std::errc::not_connected) {
        process_error(ErrorCode(errc::WRITE_ERROR, ErrorCode(err.code())));
    } else if (stats) {
        size_t size = std::accumulate(req->bufs.begin(), req->bufs.end(), size_t(0), [](size_t r, const string& s) {return r + s.size();});
        stats->bytes_out += size;
        stats->msgs_out++;
    }
}

void Connection::do_close (uint16_t code, const string& payload) {
    panda_log_verbose_debug("Connection[close]: code=" << code << ", payload:" << payload);
    bool was_connected = connected();

    if (was_connected && !parser->send_closed()) {
        auto data = parser->send_close(code, payload);
        write(data.begin(), data.end());
    }
    parser->reset();

    _state = State::INITIAL;

    if (Tcp::connected()) {
        read_stop();
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


class WSErrorCategoty : public std::error_category
{
public:
    const char * name() const noexcept override {return "unievent::websocket::Error";}
    std::string message(int ev) const override {
        switch (ev) {
        case (int)errc::READ_ERROR:    return "read error";
        case (int)errc::WRITE_ERROR:   return "write error";
        case (int)errc::CONNECT_ERROR: return "connect error";
        default: return "unknown ws error";
        }
    }
};

namespace {
struct PrettyBytes {
    size_t count;
};

std::ostream& operator<< (std::ostream& stream, const PrettyBytes& c) {
    std::array<const char*, 5> NAMES = {" B", " KiB", " MiB", " GiB", " TiB"};
    size_t val = c.count;
    size_t i = 0;
    while (val > 1200 && i < NAMES.size() - 1) {
        val /= 1024;
        ++i;
    }
    stream << val << NAMES[i];
    return stream;
}
}


std::ostream& operator<< (std::ostream& stream, const Connection::Statistics& c) {
    stream << "total " << (c.msgs_in + c.msgs_out) << "pps(" << PrettyBytes{c.bytes_in + c.bytes_out} << "/s),"
              "input " << c.msgs_in << "pps(" << PrettyBytes{c.bytes_in} << "/s),"
              "output " << c.msgs_out << "pps(" << PrettyBytes{c.bytes_out} << "/s)";
    return stream;
}

const std::error_category& ws_error_categoty = WSErrorCategoty();

}}}
