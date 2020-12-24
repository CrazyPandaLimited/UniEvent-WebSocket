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

    WriteRequestSP send (string_view payload, const Stream::write_fn& callback = {});

    Builder& opcode  (Opcode value) { MessageBuilder::opcode(value); return *this; }
    Builder& deflate (bool value)   { MessageBuilder::deflate(value); return *this; }

    template <class B, class E, class T = decltype(*std::declval<B>()), class = std::enable_if_t<std::is_convertible<T, string_view>::value>>
    WriteRequestSP send (B&& begin, E&& end, const Stream::write_fn& callback = {});

    template <class B, class E, class T = decltype(*std::declval<B>()), class = std::enable_if_t<std::is_convertible<T, string_view>::value>>
    WriteRequestSP send_multiframe (B&& begin, E&& end, const Stream::write_fn& callback = {});

    template <class B, class E, class T = decltype(*((*std::declval<B>()).begin()))>
    std::enable_if_t<std::is_convertible<T, string_view>::value, WriteRequestSP>
    send_multiframe (B&& begin, E&& end, const Stream::write_fn& callback = {});

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

    struct TcpConfig {
        bool     tcp_nodelay = false;
        uint64_t shutdown_timeout = 5000; //ms
    };

    struct Config : Parser::Config, TcpConfig {
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

    void send_message (string_view payload, const write_fn& callback = {}) {
        message().opcode(Opcode::BINARY).send(payload, callback);
    }

    template <class B, class E, class T = decltype(*std::declval<B>()), class = std::enable_if_t<std::is_convertible<T, string_view>::value>>
    void send_message (B&& begin, E&& end, const write_fn& callback = {}) {
        message().opcode(Opcode::BINARY).send(std::forward<B>(begin), std::forward<E>(end), callback);
    }

    void send_text (string& payload, const write_fn& callback = {}) {
        message().opcode(Opcode::TEXT).send(payload, callback);
    }

    template <class B, class E, class T = decltype(*std::declval<B>()), class = std::enable_if_t<std::is_convertible<T, string_view>::value>>
    void send_text (B&& begin, E&& end, const write_fn& callback = {}) {
        message().opcode(Opcode::TEXT).send(std::forward<B>(begin), std::forward<E>(end), callback);
    }

    void send_ping ();
    void send_ping (string_view payload);
    void send_pong ();
    void send_pong (string_view payload);

    void close (uint16_t code = uint16_t(CloseCode::DONE)) {
        close(code, close_message(code));
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

        Statistics fetch(double secs) {
            auto ret = normalized(secs);
            reset();
            return ret;
        }
    };
    using StatisticsSP = iptr<Statistics>;

    void stats_counter(const StatisticsSP& val) {stats = val;}
    StatisticsSP stats_counter() {return stats;}

protected:
    State _state;
    TcpConfig conf;

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
    void on_shutdown (const ErrorCode&, const ShutdownRequestSP&) override;

    void process_error (const ErrorCode& err, uint16_t code = CloseCode::ABNORMALLY);

    virtual ~Connection () = 0;

private:
    friend struct Builder;
    Parser*      parser;
    StatisticsSP stats;
    bool         _error_state;

    void process_peer_close (const MessageSP&);
};

template <class B, class E, class T = decltype(*std::declval<B>()), class = std::enable_if_t<std::is_convertible<T, string_view>::value>>
WriteRequestSP Builder::send (B&& begin, E&& end, const Stream::write_fn& callback) {
    if (!_connection.connected()) {
        if (callback) callback(&_connection, errc::WRITE_ERROR, new unievent::WriteRequest());
        return nullptr;
    }
    WriteRequestSP req = new WriteRequest(MessageBuilder::send(std::forward<B>(begin), std::forward<E>(end)));
    if (callback) req->event.add(callback);
    _connection.write(req);
    return req;
}

template <class B, class E, class T = decltype(*std::declval<B>()), class = std::enable_if_t<std::is_convertible<T, string_view>::value>>
WriteRequestSP Builder::send_multiframe (B&& begin, E&& end, const Stream::write_fn& callback) {
    if (!_connection.connected()) {
        if (callback) callback(&_connection, errc::WRITE_ERROR, new unievent::WriteRequest());
        return nullptr;
    }
    auto vdata = MessageBuilder::send_multiframe(std::forward<B>(begin), std::forward<E>(end));
    WriteRequestSP req = new WriteRequest(vdata.begin(), vdata.end());
    if (callback) req->event.add(callback);
    _connection.write(req);
    return req;
}

template <class B, class E, class T = decltype(*((*std::declval<B>()).begin()))>
std::enable_if_t<std::is_convertible<T, string_view>::value, WriteRequestSP>
Builder::send_multiframe (B&& begin, E&& end, const Stream::write_fn& callback) {
    if (!_connection.connected()) {
        if (callback) callback(&_connection, errc::WRITE_ERROR, new unievent::WriteRequest());
        return nullptr;
    }
    auto vdata = MessageBuilder::send_multiframe(std::forward<B>(begin), std::forward<E>(end));
    WriteRequestSP req = new WriteRequest(vdata.begin(), vdata.end());
    if (callback) req->event.add(callback);
    _connection.write(req);
    return req;
}

inline Connection::~Connection () {}

std::ostream& operator<< (std::ostream& stream, const Connection::Config& conf);
std::ostream& operator<< (std::ostream& stream, const Connection::Statistics& conf);

}}}
