#pragma once

#include <panda/event/TCP.h>
#include <panda/websocket/ServerParser.h>
#include <panda/CallbackDispatcher.h>
#include <panda/log.h>

namespace panda {
namespace websocket {

using panda::event::TCP;
using panda::websocket::CloseCode;
using panda::event::StreamError;
using panda::CallbackDispatcher;
using panda::event::Loop;

class BaseConnection : public TCP {
public:
    BaseConnection(Loop* loop = Loop::default_loop())
        : TCP(loop)
        , state(State::DISCONNECTED)
        , parser(nullptr)
    {}
    virtual ~BaseConnection() {panda_log_debug("~BaseConnection");}

    void init(Parser& parser) {
        this->parser = &parser;
    }

    struct Conf {
        size_t max_frame_size = 0;
        size_t max_message_size = 0;
    };

    void configure(Conf conf);

    using BaseConnectionSP = shared_ptr<BaseConnection, true>;
    CallbackDispatcher<void(BaseConnectionSP, FrameSP)>            frame_callback;
    CallbackDispatcher<void(BaseConnectionSP, MessageSP)>          message_callback;
    CallbackDispatcher<void(BaseConnectionSP, const StreamError&)> stream_error_callback;
    CallbackDispatcher<void(BaseConnectionSP, const string&)>      any_error_callback;


    template<typename... Args>
    void send_message(Args&&... args) {
        panda_log_debug("send message");
        auto all = parser->send_message(std::forward<Args>(args)..., Opcode::BINARY);
        write(all.begin(), all.end());
    }

    template<typename... Args>
    void send_text(Args&&... args) {
        panda_log_debug("send text");
        auto all = parser->send_message(std::forward<Args>(args)..., Opcode::TEXT);
        write(all.begin(), all.end());
    }

    virtual void close(uint16_t code = uint16_t(CloseCode::DONE), string payload = string());
    void close(CloseCode code, string payload = string()) {
        close(uint16_t(code), payload);
    }

protected:
    virtual void on_frame        (FrameSP frame);
    virtual void on_message      (MessageSP msg);
    virtual void on_stream_error (const StreamError& err);
    virtual void on_any_error    (const string& err);
    
    virtual void on_eof () override;

    void close_tcp();

    enum class State {
        DISCONNECTED,
        TCP_CONNECTED,
        WS_CONNECTED,
        WS_DISCONNECTED
    };
    State state;

private:
    Parser* parser;
};

}
}
