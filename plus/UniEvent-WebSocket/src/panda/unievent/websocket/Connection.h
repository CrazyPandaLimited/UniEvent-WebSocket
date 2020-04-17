#pragma once
#include <panda/log.h>
#include <panda/unievent/Tcp.h>
#include <panda/protocol/websocket/Parser.h>
#include <panda/error.h>

#include "Error.h"

namespace panda { namespace unievent { namespace websocket {

using panda::CallbackDispatcher;
using namespace panda::protocol::websocket;

struct Connection;
using ConnectionSP = iptr<Connection>;

struct Builder : private MessageBuilder {
    using MessageBuilder::opcode;
    using MessageBuilder::deflate;

    Builder (Builder&&);

    WriteRequestSP send (string& payload, const Stream::write_fn& callback = {});

    Builder& opcode  (Opcode value) { MessageBuilder::opcode(value); return *this; }
    Builder& deflate (bool value)   { MessageBuilder::deflate(value); return *this; }

    template <class Begin, class End>
    WriteRequestSP send (Begin begin, End end, const Stream::write_fn& callback = {});

protected:
    friend struct Connection;
    Connection& _connection;

private:
    Builder (Connection& connection);
};


struct Connection : Tcp, protected ITcpSelfListener {
    using message_fptr    = void(const ConnectionSP&, const MessageSP&);
    using message_fn      = function<message_fptr>;
    using error_fptr      = void(const ConnectionSP&, const ErrorCode&);
    using error_fn        = function<error_fptr>;
    using close_fptr      = void(const ConnectionSP&, uint16_t, const string&);
    using close_fn        = function<close_fptr>;
    using peer_close_fptr = void(const ConnectionSP&, const MessageSP&);
    using peer_close_fn   = function<peer_close_fptr>;
    using ping_fptr       = void(const ConnectionSP&, const MessageSP&);
    using ping_fn         = function<ping_fptr>;
    using pong_fptr       = ping_fptr;
    using pong_fn         = function<pong_fptr>;

    struct Config : Parser::Config {
        bool tcp_nodelay = false;
    };

    enum class State { INITIAL, TCP_CONNECTING, CONNECTING, CONNECTED, HALT };

    CallbackDispatcher<message_fptr>    message_event;
    CallbackDispatcher<error_fptr>      error_event;
    CallbackDispatcher<close_fptr>      close_event;
    CallbackDispatcher<peer_close_fptr> peer_close_event;
    CallbackDispatcher<ping_fptr>       ping_event;
    CallbackDispatcher<pong_fptr>       pong_event;

    Connection (const LoopSP& loop) : Tcp(loop), _state(State::INITIAL), _error_state() {
        event_listener(this);
    }

    void configure (const Config& conf);

    State state () const { return _state; }

    bool connected () const { return _state == State::CONNECTED; }
    bool connecting () const { return _state == State::CONNECTING || _state == State::TCP_CONNECTING; }

    Builder message () { return Builder(*this); }

    void send_message (string& payload, const write_fn& callback = {}) {
        message().opcode(Opcode::BINARY).send(payload, callback);
    }

    template <class Begin, class End>
    void send_message (Begin begin, End end, const write_fn& callback = {}) {
        message().opcode(Opcode::BINARY).send(begin, end, callback);
    }

    void send_text (string& payload, const write_fn& callback = {}) {
        message().opcode(Opcode::TEXT).send(payload, callback);
    }

    template <class Begin, class End>
    void send_text (Begin begin, End end, const write_fn& callback = {}) {
        message().opcode(Opcode::TEXT).send(begin, end, callback);
    }


    void send_ping (string& payload);
    void send_pong (string& payload);

    void send_ping ();
    void send_pong ();

    void close (uint16_t code = uint16_t(CloseCode::DONE)) {
        close (code, close_message(code));
    }

    void close (uint16_t code, const string& payload) {
        if (_state == State::INITIAL) return;
        do_close(code, payload);
    }

    struct Statistics : Refcnt {
        size_t msgs_in   = 0;
        size_t msgs_out  = 0;
        size_t bytes_in  = 0;
        size_t bytes_out = 0;

        Statistics() = default;
        Statistics(size_t msgs_in, size_t msgs_out, size_t bytes_in, size_t bytes_out)
            : msgs_in(msgs_in), msgs_out(msgs_out), bytes_in(bytes_in), bytes_out(bytes_out)
        {}

        void reset() {
            msgs_in = msgs_out = bytes_in = bytes_out = 0;
        }
        Statistics normalized(double secs) {
            return Statistics(msgs_in / secs, msgs_out / secs, bytes_in / secs, bytes_out / secs);
        }
    };
    using StatisticsSP = iptr<Statistics>;

    void set_statistics_counters(const StatisticsSP& val) {stats = val;}

protected:
    State _state;
    bool  _tcp_nodelay = false;

    void init (Parser& parser) { this->parser = &parser; }

    virtual void on_message    (const MessageSP&);
    virtual void on_error      (const ErrorCode&);
    virtual void on_peer_close (const MessageSP&);
    virtual void on_ping       (const MessageSP&);
    virtual void on_pong       (const MessageSP&);
    virtual void on_close      (uint16_t code, const string& payload);

    virtual void do_close (uint16_t code, const string& payload);

    void on_read  (string&, const ErrorCode&) override;
    void on_eof   () override;
    void on_write (const ErrorCode&, const WriteRequestSP&) override;

    void process_error (const ErrorCode& err, uint16_t code = CloseCode::ABNORMALLY);

    virtual ~Connection () = 0;

private:
    friend struct Builder;
    Parser*      parser;
    StatisticsSP stats;
    bool         _error_state;

    void process_peer_close (const MessageSP&);
};

template <class Begin, class End>
WriteRequestSP Builder::send (Begin begin, End end, const Stream::write_fn& callback) {
    if (!_connection.connected()) {
        if (callback) callback(&_connection, errc::WRITE_ERROR, new unievent::WriteRequest());
        return nullptr;
    }
    auto all = MessageBuilder::send(begin, end);
    WriteRequestSP req = new WriteRequest(all.begin(), all.end());
    if (callback) {
        req->event.add(callback);
    }
    _connection.write(req);
    return req;
}

inline Connection::~Connection () {}

std::ostream& operator<< (std::ostream& stream, const Connection::Config& conf);
std::ostream& operator<< (std::ostream& stream, const Connection::Statistics& conf);

}}}
