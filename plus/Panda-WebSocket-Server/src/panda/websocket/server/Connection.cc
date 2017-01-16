#include <panda/websocket/server/Server.h>
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

bool Connection::on_read (const buf_t* buf, const StreamError& err) {
    string abc(buf->base, buf->len, string::COPY);
    cout << "Connection(" << _id << ")[on_read]: " << abc << "\n";
    shutdown();
    return false;
}

Connection::~Connection () {
    cout << "connection destroy\n";
}

}}}
