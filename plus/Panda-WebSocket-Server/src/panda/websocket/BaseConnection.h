#pragma once

#include <panda/event/TCP.h>
#include <panda/websocket/ServerParser.h>
#include <panda/CallbackDispatcher.h>
#include <panda/log.h>

namespace panda {
namespace websocket {

using panda::event::TCP;
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
    CallbackDispatcher<void(BaseConnectionSP, uint16_t, string)>   close_callback;

    void send_message (string& payload, write_fn callback = {}) {
        assert(state == State::WS_CONNECTED);
        auto all = parser->send_message(payload, Opcode::BINARY);
        write(all.begin(), all.end(), callback);
    }


    template <class ContIt>
    void send_message (ContIt begin, ContIt end, write_fn callback = {}) {
        assert(state == State::WS_CONNECTED);
        auto all = parser->send_message(begin, end, Opcode::BINARY);
        write(all.begin(), all.end(), callback);
    }

    void send_text (string& payload, write_fn callback = {}) {
        assert(state == State::WS_CONNECTED);
        auto all = parser->send_message(payload, Opcode::TEXT);
        write(all.begin(), all.end(), callback);
    }


    template <class ContIt>
    void send_text (ContIt begin, ContIt end, write_fn callback = {}) {
        assert(state == State::WS_CONNECTED);
        auto all = parser->send_message(begin, end, Opcode::TEXT);
        write(all.begin(), all.end(), callback);
    }

    virtual void close(uint16_t code = uint16_t(CloseCode::DONE), string payload = string());
    virtual bool connected();

protected:
    virtual void on_frame        (FrameSP frame);
    virtual void on_message      (MessageSP msg);
    virtual void on_stream_error (const StreamError& err);
    virtual void on_any_error    (const string& err);
    
    virtual void on_eof () override;
    virtual void on_write(const event::StreamError& err, event::WriteRequest* req) override;

    void close_tcp();

    // keep the order
    enum class State {
        DISCONNECTED = 0,
        CONNECTING,
        TCP_CONNECTED,
        WS_CONNECTED,
        WS_DISCONNECTED
    };
    State state;

private:
    Parser* parser;
};

std::ostream& operator <<(std::ostream& stream, const BaseConnection::Conf& conf);

}
}
