#include "ServerConnection.h"
#include "Server.h"
#include <panda/log.h>
#include <panda/encode/base16.h>

using std::endl;
using namespace std::placeholders;

namespace panda { namespace unievent { namespace websocket {

ServerConnection::ServerConnection (Server* server, uint64_t id) : Connection(server->loop()), _id(id), _server(server), _alive(true) {
    panda_log_info("Connection[new]: id = " << _id);
    init(_parser);
}

void ServerConnection::run (Stream* listener) {
    listener->accept(this); // TODO: concurrent non-blocking accept in multi-thread may result in not accepting (err from libuv?)
    state = State::TCP_CONNECTED;
    read_start();
}

void ServerConnection::on_read (const string& buf, const CodeError* err) {
    if (err) return on_error(*err);

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

void ServerConnection::on_accept (ConnectRequestSP req) {
    ConnectResponse res;
    send_accept_response(&res);
    accept_callback(this, req);
}

void ServerConnection::send_accept_error (HTTPResponse* res) {
    string data = _parser.accept_error(res);
    write(data);
}

void ServerConnection::send_accept_response (ConnectResponse* res) {
    string data = _parser.accept_response(res);
    write(data);
}

void ServerConnection::close (uint16_t code, string payload) {
    ServerConnectionSP sp = this; // keep self from destruction if user loses all references, that how panda::unievent::TCP works
    bool call = state != State::DISCONNECTED;
    Connection::close(code, payload);
    if (call) _server->remove_connection(sp, code, payload);
}

void ServerConnection::configure (ServerConnection::Conf conf) {
    Connection::configure(conf.base);
    _parser.max_handshake_size = conf.max_handshake_size;
}

std::ostream& operator<< (std::ostream& stream, const ServerConnection::Conf& conf) {
    stream << "ServerConnection::Conf{ base:" << conf.base << ", max_handshake_size:" << conf.max_handshake_size << "}";
    return stream;
}

}}}
