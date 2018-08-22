#pragma once
#include <panda/log.h>
#include <panda/unievent/TCP.h>
#include <panda/CallbackDispatcher.h>
#include <panda/protocol/websocket/Parser.h>

namespace panda { namespace unievent { namespace websocket {

using panda::CallbackDispatcher;
using namespace panda::protocol::websocket;

struct Connection : TCP {
    struct Conf {
        size_t max_frame_size = 0;
        size_t max_message_size = 0;
    };

    using ConnectionSP = iptr<Connection>;

    Connection (Loop* loop = Loop::default_loop()) : TCP(loop), state(State::DISCONNECTED), parser(nullptr) {}

    void init (Parser& parser) {
        this->parser = &parser;
    }

    void configure (Conf conf);

    CallbackDispatcher<void(ConnectionSP, FrameSP)>            frame_callback;
    CallbackDispatcher<void(ConnectionSP, MessageSP)>          message_callback;
    CallbackDispatcher<void(ConnectionSP, const StreamError&)> stream_error_callback;
    CallbackDispatcher<void(ConnectionSP, const string&)>      any_error_callback;
    CallbackDispatcher<void(ConnectionSP, uint16_t, string)>   close_callback;

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

    virtual void close (uint16_t code = uint16_t(CloseCode::DONE), string payload = string());
    virtual bool connected ();

protected:
    virtual void on_frame        (FrameSP frame);
    virtual void on_message      (MessageSP msg);
    virtual void on_stream_error (const StreamError& err);
    virtual void on_any_error    (const string& err);

    virtual void on_eof   () override;
    virtual void on_write (const StreamError& err, WriteRequest* req) override;

    void close_tcp ();

    // keep the order
    enum class State {
        DISCONNECTED = 0,
        CONNECTING,
        TCP_CONNECTED,
        WS_CONNECTED,
        WS_DISCONNECTED
    };
    State state;

    virtual ~Connection () {}

private:
    Parser* parser;
};

using ConnectionSP = Connection::ConnectionSP;

std::ostream& operator<< (std::ostream& stream, const Connection::Conf& conf);

}}}
