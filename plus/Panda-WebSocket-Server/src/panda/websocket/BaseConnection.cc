#include "BaseConnection.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

namespace panda { namespace websocket {

void BaseConnection::close(uint16_t code, string payload) {
    panda_log_info("BaseConnection[close]: code=" << code << ", payload:" << payload);
    if (state == State::WS_CONNECTED) {
        auto data = parser->send_close(code, payload);
        write(data.begin(), data.end());
        state = State::WS_DISCONNECTED;
    }
    close_tcp();
}

void BaseConnection::on_frame(FrameSP frame) {
    if (Log::should_log(Log::DEBUG, _panda_code_point_)){
        Log logger = Log(_panda_code_point_, Log::DEBUG);
        logger << "websocket BaseConnection::on_frame: payload=\n";
        for (const auto& str : frame->payload) {
            logger << encode::encode_base16(str);
        }
    }
    frame_callback(this, frame);
}

void BaseConnection::on_message(MessageSP msg) {
    if (Log::should_log(Log::DEBUG, _panda_code_point_)){
        Log logger = Log(_panda_code_point_, Log::DEBUG);
        logger << "websocket BaseConnection::on_message: payload=\n";
        for (const auto& str : msg->payload) {
            logger << encode::encode_base16(str);
        }
    }
    message_callback(this, msg);
}

void BaseConnection::on_stream_error(const event::StreamError& err) {
    panda_log_info("on_stream_error: " << err.what());
    stream_error_callback(this, err);
    on_any_error(err.what());
}

void BaseConnection::on_any_error(const string& err) {
    panda_log_warn(err);
    any_error_callback(this, err);
    close(CloseCode::ABNORMALLY);
}

void BaseConnection::on_eof() {
    panda_log_info("on_eof");
    state = State::DISCONNECTED;
    close();
}

void BaseConnection::close_tcp() {
    if (state == State::DISCONNECTED) {
        return;
    }
    shutdown();
    disconnect();
    state = State::DISCONNECTED;
}


}}
