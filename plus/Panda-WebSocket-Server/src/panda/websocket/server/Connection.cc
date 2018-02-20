#include <panda/websocket/server/Connection.h>
#include <panda/websocket/server/Server.h>
#include <panda/log.h>
#include <panda/encode/base16.h>

namespace panda { namespace websocket { namespace server {

using std::endl;
using namespace std::placeholders;

Connection::Connection (Server* server, uint64_t id)
    : BaseConnection(server->loop())
    , _id(id)
    , _server(server)
    , _alive(true)
{
    panda_log_info("Connection[new]: id = " << _id);
    init(_parser);
}

void Connection::run (Stream* listener) {
    listener->accept(this); // TODO: concurrent non-blocking accept in multi-thread may result in not accepting (err from libuv?)
    state = State::TCP_CONNECTED;
    read_start();
}

void Connection::on_read (const string& buf, const StreamError& err) {
    if (err) return on_stream_error(err);

    panda_log_debug("Connection(" << _id << ")[on_read]: " << encode::encode_base16(buf));

    string chunk = buf;

    if (!_parser.accept_parsed()) {
        auto creq = _parser.accept(chunk);
        if (!creq) return;

        if (creq->error) {
            panda_log_info(creq->error);
            HTTPResponse res;
            send_accept_error(&res);
            close();
        }
        else {
            state = State::WS_CONNECTED;
            on_accept(creq);
        }

        return;
    }

    if (!_parser.accepted()) throw "should not happen";

    auto msg_range = _parser.get_messages(chunk);
    for (const auto& msg : msg_range) {
        if (msg->error) return close(CloseCode::PROTOCOL_ERROR);
        if (msg->opcode() == Opcode::CLOSE) {
            panda_log_debug("connection(" << id() << ") closed by client:" << msg->close_code());
            return close(msg->close_code());
        }
        if (msg->opcode() == Opcode::PING) write(_parser.send_pong());
        on_message(msg);
        if (state != State::WS_CONNECTED) {
            break;
        }
    }
}

void Connection::on_accept (ConnectRequestSP req) {
    panda_log_info("Connection(" << _id << ")[on_accept]: req=" << req->uri->to_string());
    ConnectResponse res;
    send_accept_response(&res);
    accept_callback(this, req);
}

void Connection::send_accept_error (HTTPResponse* res) {
    string data = _parser.accept_error(res);
    panda_log_info("Connection(" << _id << ")[send_accept_error]: sending\n" << data);
    write(data);
}

void Connection::send_accept_response (ConnectResponse* res) {
    string data = _parser.accept_response(res);
    panda_log_info("Connection(" << _id << ")[send_accept_response]: sending\n" << data);
    write(data);
}

void Connection::close(uint16_t code, string payload)
{
    ConnectionSP sp = this; // keep self from destruction if user loses all references, that how panda::event::TCP works
    if (state != State::DISCONNECTED) {
        _server->remove_connection(sp, code, payload);
    }
    BaseConnection::close(code, payload);
}

Connection::~Connection () {
    panda_log_info("connection destroy");
}

void Connection::configure(Connection::Conf conf) {
    BaseConnection::configure(conf.base);
    _parser.max_handshake_size = conf.max_handshake_size;
}

}}}
