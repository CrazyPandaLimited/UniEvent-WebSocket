#include "Connection.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

using namespace panda::unievent::websocket;

void Connection::configure (Connection::Conf conf) {
    parser->max_frame_size = conf.max_frame_size;
    parser->max_message_size = conf.max_message_size;
}

void Connection::close (uint16_t code, string payload) {
    panda_log_info("Connection[close]: state=" << (int)state << " code=" << code << ", payload:" << payload);
    if (state == State::WS_CONNECTED) {
        auto data = parser->send_close(code, payload);
        write(data.begin(), data.end());
        state = State::WS_DISCONNECTED;
    }
    if (state != State::DISCONNECTED) {
        bool call = state >= State::WS_CONNECTED;
        close_tcp();
        if (call) {
            close_callback(this, code, payload);
        }
    } 
}

bool Connection::connected () {
    return state == State::WS_CONNECTED;
}

void Connection::on_frame (FrameSP frame) {
    if (Log::should_log(logger::VERBOSE_DEBUG, _panda_code_point_)){
        Log logger = Log(_panda_code_point_, logger::VERBOSE_DEBUG);
        logger << "websocket BaseConnection::on_frame: payload=\n";
        for (const auto& str : frame->payload) {
            logger << encode::encode_base16(str);
        }
    }
    frame_callback(this, frame);
}

void Connection::on_message (MessageSP msg) {
    if (Log::should_log(logger::VERBOSE_DEBUG, _panda_code_point_)){
        Log logger = Log(_panda_code_point_, logger::VERBOSE_DEBUG);
        logger << "websocket BaseConnection::on_message: payload=\n";
        for (const auto& str : msg->payload) {
            logger << encode::encode_base16(str);
        }
    }
    message_callback(this, msg);
}

void Connection::on_stream_error (const StreamError& err) {
    panda_log_info("websocket on_stream_error: " << err.what());
    stream_error_callback(this, err);
    on_any_error(err.what());
    close_reinit(true);
}

void Connection::on_any_error (const string& err) {
    panda_log_warn(err);
    any_error_callback(this, err);
    close(CloseCode::ABNORMALLY);
}

void Connection::on_eof () {
    panda_log_info("websocket on_eof");
    if (state == State::WS_CONNECTED) {
        close(CloseCode::ABNORMALLY);
    }
    TCP::on_eof();
}

void Connection::on_write (const StreamError& err, WriteRequest* req) {
    TCP::on_write(err, req);
    if (err) on_any_error(err.what());
}

void Connection::close_tcp () {
    shutdown();
    disconnect();
    state = State::DISCONNECTED;
}

std::ostream& operator<< (std::ostream& stream, const Connection::Conf& conf) {
    stream << "WebSocket::BaseConf{max_frame_size:" << conf.max_frame_size
           << ", max_message_size:" << conf.max_message_size
           << "}";
    return stream;
}
