#include "Connection.h"
#include <panda/encode/base16.h>
#include <numeric>

namespace panda { namespace unievent { namespace websocket {

static const auto& panda_log_module = uewslog;
using protocol::websocket::ccfmt;

Builder::Builder (Builder&& b) : MessageBuilder(std::move(b)), _connection{b._connection} {}

Builder::Builder (Connection& connection) : MessageBuilder(connection.parser->message()), _connection{connection} {}

WriteRequestSP Builder::send(string& payload, const Stream::write_fn& callback) {
    if (!_connection.connected()) {
        if (callback) callback(&_connection, errc::WRITE_ERROR, new unievent::WriteRequest());
        return nullptr;
    }
    auto all = MessageBuilder::send(payload);
    WriteRequestSP req = new WriteRequest(all.begin(), all.end());
    if (callback) req->event.add(callback);
    _connection.write(req);
    return req;
}

void Connection::configure (const Config& conf) {
    parser->configure(conf);
    this->conf = conf;
}

static void log_use_after_close () {
    panda_log_info("using websocket::Connection after close");
}

void Connection::on_read (string& buf, const ErrorCode& err) {
    panda_log_debug("Websocket on read:" << log::escaped{buf});
    assert(_state == State::CONNECTED && parser->established());
    if (err) return process_error(nest_error(errc::READ_ERROR, err));
    if (stats) {
        stats->bytes_in += buf.size();
        stats->msgs_in++;
    }

    auto msg_range = parser->get_messages(buf);
    for (const auto& msg : msg_range) {
        if (msg->error) {
            panda_log_notice("protocol error: " << msg->error);
            process_error(nest_error(errc::READ_ERROR, msg->error), parser->suggested_close_code());
            break;
        }
        switch (msg->opcode()) {
            case Opcode::CLOSE:
                panda_log_notice("connection closed by peer:" << ccfmt(msg->close_code(), msg->close_message()));
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
    auto suggested_code = parser->suggested_close_code();
    on_peer_close(msg);
    if (_error_state) {
        if (msg) close(suggested_code, msg->close_message());
        else     close(CloseCode::ABNORMALLY);
    }
}

void Connection::on_peer_close (const MessageSP& msg) {
    peer_close_event(this, msg);
}

void Connection::on_ping (const MessageSP& msg) {
    if (msg->payload_length() > Frame::MAX_CONTROL_PAYLOAD) {
        panda_log_notice("something weird, ping payload is bigger than possible");
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
    panda_log_notice("websocket error: " << err.message());
    if (_state == State::INITIAL) return; // just ignore everything, we are here after close
    _error_state = true;
    on_error(err);
    if (_error_state) close(code);
}

void Connection::on_error (const ErrorCode& err) {
    error_event(this, err);
}

void Connection::on_eof () {
    panda_log_notice("websocket on_eof");
    process_peer_close(nullptr);
}

void Connection::on_write (const ErrorCode& err, const WriteRequestSP& req) {
    panda_log_debug("websocket on_write: " << err);
    if (err && err != std::errc::operation_canceled && err != std::errc::broken_pipe && err != std::errc::not_connected) {
        process_error(nest_error(errc::WRITE_ERROR, err));
    } else if (stats) {
        size_t size = std::accumulate(req->bufs.begin(), req->bufs.end(), size_t(0), [](size_t r, const string& s) {return r + s.size();});
        stats->bytes_out += size;
        stats->msgs_out++;
    }
}

void websocket::Connection::on_shutdown(const ErrorCode& err, const ShutdownRequestSP&) {
    panda_log_debug("websocket on_shutdown " << err);
    if (err == std::errc::timed_out) {
        reset();
    }
}

void Connection::do_close (uint16_t code, const string& payload) {
    panda_log_debug("Connection[close]: code=" << ccfmt(code, payload));
    bool was_connected = connected();

    //in_connected, not out. it checks if we are in eof callback
    if (Tcp::in_connected() && was_connected && !parser->send_closed()) {
        auto data = parser->send_close(code, payload);
        write(data.begin(), data.end());
    }
    parser->reset();

    _state = State::INITIAL;

    if (Tcp::connected()) {
        read_stop();
        shutdown(conf.shutdown_timeout);
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


}}}
