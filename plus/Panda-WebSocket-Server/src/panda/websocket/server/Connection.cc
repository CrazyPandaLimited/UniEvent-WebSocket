#include <panda/websocket/server/Connection.h>
#include <panda/websocket/server/Server.h>

namespace panda { namespace websocket { namespace server {

using std::cout;
using namespace std::placeholders;

Connection::Connection (Server* server, uint64_t id) : TCP(server->loop()), _id(id), _server(server) {
    cout << "Connection[new]: id = " << _id << "\n";
}

void Connection::run (Stream* listener) {
    listener->accept(this); // TODO: concurrent non-blocking accept in multi-thread may result in not accepting (err from libuv?)
    read_start();
}

void Connection::on_read (const string& buf, const StreamError& err) {
    if (err) return on_stream_error(err);

    cout << "Connection(" << _id << ")[on_read]: " << buf << "\n";

    string chunk = buf;

    if (!_parser.accept_parsed()) {
        auto creq = _parser.accept(chunk);
        if (!creq) return;

        if (creq->error) {
            HTTPResponse res;
            send_accept_error(&res);
            shutdown();
            _server->remove_connection(this);
        }
        else on_accept(creq);

        return;
    }

    if (!_parser.accepted()) throw "should not happen";

    auto msg_range = _parser.get_messages(chunk);
    for (const auto& msg : msg_range) {
        if (msg->error) return close(PROTOCOL_ERROR);
        on_message(msg);
    }
}

void Connection::on_eof () {

}

void Connection::on_stream_error (const StreamError& err) {
    cout << "Connection(" << _id << ")[on_read]: error " << err.what() << "\n";
    if (stream_error_callback) stream_error_callback(this, err);
    else close(AWAY);
}

void Connection::on_accept (ConnectRequestSP req) {
    cout << "Connection(" << _id << ")[on_accept]: req=" << req->uri->to_string() << "\n";
    ConnectResponse res;
    send_accept_response(&res);
}

void Connection::send_accept_error (HTTPResponse* res) {
    string data = _parser.accept_error(res);
    cout << "Connection(" << _id << ")[send_accept_error]: sending\n" << data << "\n";
    write(data);
    shutdown();
    _server->remove_connection(this);
}

void Connection::send_accept_response (ConnectResponse* res) {
    string data = _parser.accept_response(res);
    cout << "Connection(" << _id << ")[send_accept_response]: sending\n" << data << "\n";
    write(data);
}

void Connection::close (int code) {
    string data = _parser.send_close(code);
    cout << "Connection(" << _id << ")[close]: code=\n" << code << "\n";
    write(data);
    shutdown();
    _server->remove_connection(this);
}

void Connection::on_frame (FrameSP frame) {
    cout << "Connection(" << _id << ")[on_frame]: payload=\n";
    for (const auto& str : frame->payload) {
        cout << str;
    }
    cout << "\n";
    if (frame_callback) frame_callback(this, frame);
}

void Connection::on_message (MessageSP msg) {
    cout << "Connection(" << _id << ")[on_message]: payload=\n";
    for (const auto& str : msg->payload) {
        cout << str;
    }
    cout << "\n";
    if (message_callback) message_callback(this, msg);
}

Connection::~Connection () {
    cout << "connection destroy\n";
}

}}}
