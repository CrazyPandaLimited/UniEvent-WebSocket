#pragma once
#include <panda/log.h>
#include <panda/unievent/TCP.h>
#include <panda/CallbackDispatcher.h>
#include <panda/protocol/websocket/Parser.h>

namespace panda { namespace unievent { namespace websocket {

using panda::CallbackDispatcher;
using namespace panda::protocol::websocket;

struct ConnectionBase : TCP {
    struct Config {
        size_t max_frame_size = 0;
        size_t max_message_size = 0;
    };

    using SP = iptr<ConnectionBase>;

    ConnectionBase (Loop* loop = Loop::default_loop()) : TCP(loop), state(State::DISCONNECTED), parser(nullptr) {}

    void init (Parser& parser) {
        this->parser = &parser;
    }

    void configure (const Config& conf);

    CallbackDispatcher<void(SP, FrameSP)>          frame_callback;
    CallbackDispatcher<void(SP, MessageSP)>        message_callback;
    CallbackDispatcher<void(SP, const Error&)>     error_callback;
    CallbackDispatcher<void(SP, uint16_t, string)> close_callback;

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
    virtual void on_frame   (FrameSP frame);
    virtual void on_message (MessageSP msg);
    virtual void on_error   (const Error& err);

    virtual void on_eof   () override;
    virtual void on_write (const CodeError* err, WriteRequest* req) override;

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

    virtual ~ConnectionBase () = 0;

private:
    Parser* parser;
};

inline ConnectionBase::~ConnectionBase () {}

using ConnectionBaseSP = ConnectionBase::SP;

std::ostream& operator<< (std::ostream& stream, const ConnectionBase::Config& conf);

}}}
