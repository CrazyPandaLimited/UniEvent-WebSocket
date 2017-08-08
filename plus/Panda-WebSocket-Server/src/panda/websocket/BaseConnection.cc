#include "BaseConnection.h"
#include <panda/log.h>

namespace panda { namespace websocket {

void BaseConnection::close(uint16_t code, string payload) {
    panda_log_info("BaseConnection[close]: code=" << code);
    if (state == State::WS_CONNECTED) {
        panda_log_info("BaseConnection[close]: sending close payload=" << payload);
        auto data = parser.send_close(code, payload);
        write(data.begin(), data.end());
        state = State::WS_DISCONNECTED;
    }
    close_tcp();
}

void BaseConnection::on_frame(FrameSP frame) {
    {
        Log logger(Log::VERBOSE);
        logger << "BaseConnection::on_frame: payload=\n";
        for (const auto& str : frame->payload) {
            logger << str;
        }
    }
    frame_callback(this, frame);
}

void BaseConnection::on_message(MessageSP msg) {
    {
        Log logger(Log::VERBOSE);
        logger << "BaseConnection::on_message: payload=\n";
        for (const auto& str : msg->payload) {
            logger << str;
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
    state = State::WS_DISCONNECTED;
    close();
}

void BaseConnection::close_tcp() {
//    message_callback.remove_all();
//    frame_callback.remove_all();
    if (state == State::DISCONNECTED) {
        return;
    }
    panda_log_debug("shutdown");
    shutdown();
    panda_log_debug("shutdown done");
    state = State::DISCONNECTED;
}


}}
