#pragma once
#include <panda/log.h>
#include <panda/unievent/TCP.h>
#include <panda/CallbackDispatcher.h>
#include <panda/protocol/websocket/Parser.h>

namespace panda { namespace unievent { namespace websocket {

using panda::CallbackDispatcher;
using namespace panda::protocol::websocket;

struct Connection : TCP {
    struct Config {
        size_t max_handshake_size = 0;
        size_t max_frame_size     = 0;
        size_t max_message_size   = 0;
    };

    using SP = iptr<Connection>;

    Connection (Loop* loop) : TCP(loop), parser(nullptr), valid(false) {}

    void configure (const Config& conf);

    virtual bool connected () const;

    CallbackDispatcher<void(SP, FrameSP)>          frame_event;
    CallbackDispatcher<void(SP, MessageSP)>        message_event;
    CallbackDispatcher<void(SP, const Error&)>     error_event;
    CallbackDispatcher<void(SP, uint16_t, string)> close_event;
    CallbackDispatcher<void(SP, MessageSP)>        peer_close_event;
    CallbackDispatcher<void(SP, MessageSP)>        ping_event;
    CallbackDispatcher<void(SP, MessageSP)>        pong_event;

    void send_message (string& payload, write_fn callback = {}) {
        auto all = parser->send_message(payload, Opcode::BINARY);
        write(all.begin(), all.end(), callback);
    }

    template <class ContIt>
    void send_message (ContIt begin, ContIt end, write_fn callback = {}) {
        auto all = parser->send_message(begin, end, Opcode::BINARY);
        write(all.begin(), all.end(), callback);
    }

    void send_text (string& payload, write_fn callback = {}) {
        auto all = parser->send_message(payload, Opcode::TEXT);
        write(all.begin(), all.end(), callback);
    }

    template <class ContIt>
    void send_text (ContIt begin, ContIt end, write_fn callback = {}) {
        auto all = parser->send_message(begin, end, Opcode::TEXT);
        write(all.begin(), all.end(), callback);
    }

    void send_ping (string& payload);
    void send_pong (string& payload);

    void send_ping () { write(parser->send_ping()); }
    void send_pong () { write(parser->send_ping()); }

    virtual void close (uint16_t code = uint16_t(CloseCode::DONE), const string& payload = string());

protected:
    void init (Parser& parser) { this->parser = &parser; valid = true; }

    virtual void on_frame      (FrameSP frame);
    virtual void on_message    (MessageSP msg);
    virtual void on_error      (const Error& err);
    virtual void on_peer_close (MessageSP msg);
    virtual void on_ping       (MessageSP msg);
    virtual void on_pong       (MessageSP msg);
    virtual void on_close      (uint16_t code, const string& payload);

    void on_read  (string& buf, const CodeError* err) override;
    void on_eof   () override;
    void on_write (const CodeError* err, WriteRequest* req) override;

    void close_tcp ();

    bool is_valid();

    virtual ~Connection () = 0;

private:
    Parser* parser;
    bool valid;
};

inline Connection::~Connection () {}

using ConnectionSP = Connection::SP;

std::ostream& operator<< (std::ostream& stream, const Connection::Config& conf);

}}}
