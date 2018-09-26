#include "ConnectionBase.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

namespace panda { namespace unievent { namespace websocket {

void ConnectionBase::configure (const Config& conf) {
    parser->max_handshake_size = conf.max_handshake_size;
    parser->max_frame_size     = conf.max_frame_size;
    parser->max_message_size   = conf.max_message_size;
}

bool ConnectionBase::connected () const { return parser->established() && !parser->send_closed(); }

void ConnectionBase::on_read (string& buf, const CodeError* err) {
    assert(parser->established());
    if (err) return on_error(*err);
    auto msg_range = parser->get_messages(buf);
    for (const auto& msg : msg_range) {
        if (msg->error) return close(CloseCode::PROTOCOL_ERROR);
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

void ConnectionBase::on_frame (FrameSP frame) {
    if (Log::should_log(logger::VERBOSE_DEBUG, _panda_code_point_)){
        Log logger = Log(_panda_code_point_, logger::VERBOSE_DEBUG);
        logger << "websocket ConnectionBase::on_frame: payload=\n";
        for (const auto& str : frame->payload) logger << encode::encode_base16(str);
    }
    frame_event(this, frame);
}

void ConnectionBase::on_message (MessageSP msg) {
    if (Log::should_log(logger::VERBOSE_DEBUG, _panda_code_point_)){
        Log logger = Log(_panda_code_point_, logger::VERBOSE_DEBUG);
        logger << "websocket ConnectionBase::on_message: payload=\n";
        for (const auto& str : msg->payload) logger << encode::encode_base16(str);
    }
    message_event(this, msg);
}

void ConnectionBase::send_ping (string& payload) {
    auto all = parser->send_ping(payload);
    write(all.begin(), all.end());
}

void ConnectionBase::send_pong (string& payload) {
    auto all = parser->send_pong(payload);
    write(all.begin(), all.end());
}

void ConnectionBase::on_peer_close (MessageSP msg) {
    peer_close_event(this, msg);
    close(msg->close_code(), msg->close_message());
}

void ConnectionBase::on_ping (MessageSP msg) {
    ping_event(this, msg);
    switch (msg->payload.size()) {
        case 0: send_pong(); break;
        case 1: send_pong(msg->payload.front()); break;
        default:
            string acc;
            for (const auto& str : msg->payload) acc += str;
            send_pong(acc);
    }
}

void ConnectionBase::on_pong (MessageSP msg) {
    pong_event(this, msg);
}

void ConnectionBase::on_error (const Error& err) {
    panda_log_info("websocket on_error: " << err.whats());
    error_event(this, err);
    close(CloseCode::ABNORMALLY);
    close_reinit(true);
}

void ConnectionBase::on_eof () {
    panda_log_info("websocket on_eof");
    if (parser->established()) close(CloseCode::ABNORMALLY);
    TCP::on_eof();
}

void ConnectionBase::on_write (const CodeError* err, WriteRequest* req) {
    TCP::on_write(err, req);
    if (err) on_error(*err);
}

void ConnectionBase::close (uint16_t code, const string& payload) {
    panda_log_info("ConnectionBase[close]: code=" << code << ", payload:" << payload);
    bool established = parser->established();
    if (established && !parser->send_closed()) {
        auto data = parser->send_close(code, payload);
        write(data.begin(), data.end());
    }
    parser->reset();
    if (TCP::connecting() || TCP::connected()) close_tcp();
    if (established) on_close(code, payload);
}

void ConnectionBase::on_close (uint16_t code, const string& payload) {
    close_event(this, code, payload);
}

void ConnectionBase::close_tcp () {
    shutdown();
    disconnect();
}

std::ostream& operator<< (std::ostream& stream, const ConnectionBase::Config& conf) {
    stream << "ConnectionBase::Conf{max_frame_size:" << conf.max_frame_size
           << ", max_message_size:" << conf.max_message_size
           << "}";
    return stream;
}

}}}
