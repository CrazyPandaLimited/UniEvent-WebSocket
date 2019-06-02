#pragma once
#include <panda/log.h>
#include <panda/unievent/Tcp.h>
#include <panda/protocol/websocket/Parser.h>

namespace panda { namespace unievent { namespace websocket {

using panda::CallbackDispatcher;
using namespace panda::protocol::websocket;

struct Connection;
using ConnectionSP = iptr<Connection>;

struct Builder : private MessageBuilder {
    using MessageBuilder::opcode;
    using MessageBuilder::deflate;

    Builder (Builder&&);

    void send (string& payload, const Stream::write_fn& callback = {});

    Builder& opcode  (Opcode value) { MessageBuilder::opcode(value); return *this; }
    Builder& deflate (bool value)   { MessageBuilder::deflate(value); return *this; }

    template <class ContIt>
    void send (ContIt begin, ContIt end, const Stream::write_fn& callback = {});

protected:
    friend struct Connection;
    Connection& _connection;

private:
    Builder (Connection& connection);
};


struct Connection : Tcp {
    struct Config: public Parser::Config {};

    Connection (const LoopSP& loop) : Tcp(loop), parser(nullptr), valid(false) {}

    void configure (const Config& conf);

    virtual bool connected () const;

    CallbackDispatcher<void(const ConnectionSP&, const FrameSP&)>          frame_event;
    CallbackDispatcher<void(const ConnectionSP&, const MessageSP&)>        message_event;
    CallbackDispatcher<void(const ConnectionSP&, const Error&)>            error_event;
    CallbackDispatcher<void(const ConnectionSP&, uint16_t, const string&)> close_event;
    CallbackDispatcher<void(const ConnectionSP&, const MessageSP&)>        peer_close_event;
    CallbackDispatcher<void(const ConnectionSP&, const MessageSP&)>        ping_event;
    CallbackDispatcher<void(const ConnectionSP&, const MessageSP&)>        pong_event;

    Builder message () { return Builder(*this); }

    void send_message (string& payload, const write_fn& callback = {}) {
        message().opcode(Opcode::BINARY).send(payload, callback);
    }

    template <class ContIt>
    void send_message (ContIt begin, ContIt end, const write_fn& callback = {}) {
        message().opcode(Opcode::BINARY).send(begin, end, callback);
    }

    void send_text (string& payload, const write_fn& callback = {}) {
        message().opcode(Opcode::TEXT).send(payload, callback);
    }

    template <class ContIt>
    void send_text (ContIt begin, ContIt end, const write_fn& callback = {}) {
        message().opcode(Opcode::TEXT).send(begin, end, callback);
    }


    void send_ping (string& payload);
    void send_pong (string& payload);

    void send_ping () { write(parser->send_ping()); }
    void send_pong () { write(parser->send_ping()); }

    virtual void close (uint16_t code = uint16_t(CloseCode::DONE), const string& payload = string());

protected:
    void init (Parser& parser) { this->parser = &parser; valid = true; }

    virtual void on_frame      (const FrameSP&);
    virtual void on_message    (const MessageSP&);
    virtual void on_error      (const Error&);
    virtual void on_peer_close (const MessageSP&);
    virtual void on_ping       (const MessageSP&);
    virtual void on_pong       (const MessageSP&);
    virtual void on_close      (uint16_t code, const string& payload);

    void on_read  (string&, const CodeError&) override;
    void on_eof   () override;
    void on_write (const CodeError&, const WriteRequestSP&) override;

    void close_tcp ();

    bool is_valid () const { return valid; }

    virtual ~Connection () = 0;

private:
    friend struct Builder;
    Parser* parser;
    bool    valid;
};

template <class ContIt>
void Builder::send (ContIt begin, ContIt end, const Stream::write_fn& callback) {
    auto all = MessageBuilder::send(begin, end);
    _connection.write(all.begin(), all.end(), callback);
}

inline Connection::~Connection () {}

std::ostream& operator<< (std::ostream& stream, const Connection::Config& conf);

}}}
