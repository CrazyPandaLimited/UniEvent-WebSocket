#include "BaseConnection.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

namespace panda { namespace websocket {

void BaseConnection::configure(BaseConnection::Conf conf) {
    parser->max_frame_size = conf.max_frame_size;
    parser->max_message_size = conf.max_message_size;
}

void BaseConnection::close(uint16_t code, string payload) {
    panda_log_info("BaseConnection[close]: code=" << code << ", payload:" << payload);
    if (state == State::WS_CONNECTED) {
        auto data = parser->send_close(code, payload);
        write(data.begin(), data.end());
        state = State::WS_DISCONNECTED;
    }
    if (state != State::DISCONNECTED) {
        close_tcp();
        close_callback(this, code, payload);
    }
}

bool BaseConnection::connected() {
    return state == State::WS_CONNECTED;
}

void BaseConnection::on_frame(FrameSP frame) {
    if (Log::should_log(logger::DEBUG, _panda_code_point_)){
        Log logger = Log(_panda_code_point_, logger::DEBUG);
        logger << "websocket BaseConnection::on_frame: payload=\n";
        for (const auto& str : frame->payload) {
            logger << encode::encode_base16(str);
        }
    }
    frame_callback(this, frame);
}

void BaseConnection::on_message(MessageSP msg) {
    if (Log::should_log(logger::VERBOSE_DEBUG, _panda_code_point_)){
        Log logger = Log(_panda_code_point_, logger::VERBOSE_DEBUG);
        logger << "websocket BaseConnection::on_message: payload=\n";
        for (const auto& str : msg->payload) {
            logger << encode::encode_base16(str);
        }
    }
    message_callback(this, msg);
}

void BaseConnection::on_stream_error(const event::StreamError& err) {
    panda_log_info("websocket on_stream_error: " << err.what());
    stream_error_callback(this, err);
    on_any_error(err.what());
}

void BaseConnection::on_any_error(const string& err) {
    panda_log_warn(err);
    any_error_callback(this, err);
    close(CloseCode::ABNORMALLY);
}

void BaseConnection::on_eof() {
    panda_log_info("websocket on_eof");
    state = State::DISCONNECTED;
    close();
    TCP::on_eof();
}

void BaseConnection::close_tcp() {
    shutdown();
    disconnect();
    state = State::DISCONNECTED;
}


}}
