#include <panda/websocket/server/Connection.h>
#include <panda/websocket/server/Server.h>
#include <panda/log.h>

namespace panda { namespace websocket { namespace server {

using std::endl;
using namespace std::placeholders;

Connection::Connection (Server* server, uint64_t id) : TCP(server->loop()), _id(id), _server(server), _alive(true) {
    panda_log_info("Connection[new]: id = " << _id);
}

void Connection::run (Stream* listener) {
    listener->accept(this); // TODO: concurrent non-blocking accept in multi-thread may result in not accepting (err from libuv?)
    read_start();
}

void Connection::on_read (const string& buf, const StreamError& err) {
    if (err) return on_stream_error(err);

    panda_log_verbose("Connection(" << _id << ")[on_read]: " << buf);

    string chunk = buf;

    if (!_parser.accept_parsed()) {
        auto creq = _parser.accept(chunk);
        if (!creq) return;

        if (creq->error) {
            HTTPResponse res;
            send_accept_error(&res);
            close();
        }
        else {
            on_accept(creq);
        }

        return;
    }

    if (!_parser.accepted()) throw "should not happen";

    auto msg_range = _parser.get_messages(chunk);
    for (const auto& msg : msg_range) {
        if (msg->error) return close(CloseCode::PROTOCOL_ERROR);
        if (msg->opcode() == Opcode::CLOSE) return close(msg->close_code());
        if (msg->opcode() == Opcode::PING) return write(_parser.send_pong());
        on_message(msg);
    }
}

void Connection::close()
{
    if (_alive) {
        shutdown();
        _server->remove_connection(this);
        _alive = false;
    }
}

void Connection::on_eof () {
    panda_log_info("Connection(" << _id << ")[on_eof]");
    close();

}

void Connection::on_stream_error (const StreamError& err) {
    panda_log_info("Connection(" << _id << ")[on_read]: error " << err.what());
    stream_error_callback(this, err);
    close(CloseCode::AWAY);
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

void Connection::close (int code) {
    auto data = _parser.send_close(code);
    panda_log_info("Connection(" << _id << ")[close]: code=" << code);
    write(data.begin(), data.end());
    close();
}

void Connection::on_frame (FrameSP frame) {
    {
        Log logger(Log::VERBOSE);
        logger << "Connection(" << _id << ")[on_frame]: payload=\n";
        for (const auto& str : frame->payload) {
            logger << str;
        }
    }
    frame_callback(this, frame);
}

void Connection::on_message (MessageSP msg) {
    {
        Log logger(Log::VERBOSE);
        logger << "Connection(" << _id << ")[on_message]: payload=\n";
        for (const auto& str : msg->payload) {
            logger << str;
        }
    }
    message_callback(this, msg);
}

Connection::~Connection () {
    panda_log_info("connection destroy");
}

}}}
