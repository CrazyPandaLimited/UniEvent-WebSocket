#include <panda/websocket/server/Connection.h>

namespace panda { namespace websocket { namespace server {

using std::cout;
using namespace std::placeholders;

Connection::Connection (Loop* loop, uint64_t id) : TCP(loop), _id(id) {
    cout << "Connection[new]: id = " << _id << "\n";
}

void Connection::run (Stream* listener) {
    listener->accept(this);
    //read_callback = std::bind(&Connection::on_read, this, _1, _2, _3);
    //eof_callback  = std::bind(&Connection::on_eof, )
}

void Connection::on_buf_alloc (buf_t* buf) {
    buf->base = (char*)malloc(buf->len);
    if (!buf->base) throw std::bad_alloc();
}

void Connection::on_buf_free (const buf_t* buf) {
    free(buf->base);
}

static void _buf_dtor (char* ptr, size_t) {
    free(ptr);
}

bool Connection::on_read (const buf_t* buf, const StreamError& err) {
    if (err) {
        cout << "Connection(" << _id << ")[on_read]: error " << err << "\n";
        throw "pizdec";
        return false;
    }

    string chunk(buf->base, buf->len, buf->len, _buf_dtor);
    cout << "Connection(" << _id << ")[on_read]: " << chunk << "\n";

    if (!_parser.accept_parsed()) {
        auto creq = _parser.accept(chunk);
        if (!creq) return true;

        if (creq->error) {
            HTTPResponse res;
            ws_accept_error(&res);
        }
        else on_ws_accept(creq);

        return true;
    }

    if (!_parser.accepted()) throw "should not happen";

    auto msg_range = _parser.get_messages(chunk);
    for (const auto& msg : msg_range) {
        on_ws_message(msg);
    }

    return true;
}

void Connection::on_ws_accept (ConnectRequestSP req) {
    cout << "Connection(" << _id << ")[on_ws_accept]: req=" << req->uri->to_string() << "\n";
    ConnectResponse res;
    ws_accept_response(&res);
}

void Connection::ws_accept_error (HTTPResponse* res) {
    string data = _parser.accept_error(res);
    cout << "Connection(" << _id << ")[ws_accept_error]: sending\n" << data << "\n";
    write(data.buf(), data.length());
}

void Connection::ws_accept_response (ConnectResponse* res) {
    string data = _parser.accept_response(res);
    cout << "Connection(" << _id << ")[ws_accept_response]: sending\n" << data << "\n";
    write(data.buf(), data.length());
}

void Connection::on_ws_message (MessageSP msg) {
    cout << "Connection(" << _id << ")[on_ws_message]: payload=\n";
    for (const auto& str : msg->payload) {
        cout << str;
    }
    cout << "\n";
}

Connection::~Connection () {
    cout << "connection destroy\n";
}

}}}
