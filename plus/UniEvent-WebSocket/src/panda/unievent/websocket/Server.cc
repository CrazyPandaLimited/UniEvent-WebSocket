#include "Server.h"
#include <panda/log.h>

using namespace std::placeholders;
using namespace panda::unievent::websocket;

std::atomic<uint64_t> Server::lastid(0);

Server::Server (Loop* loop) : _loop(loop), running(false) {
    panda_log_info("Server(): loop is default = " << (_loop == Loop::default_loop()));
}

void Server::init (ServerConfig config) {
    if (!config.locations.size()) throw std::invalid_argument("no locations to listen supplied");

    for (auto& loc : config.locations) {
        if (!loc.host)    throw std::invalid_argument("empty host in one of locations");
        if (!loc.port)    throw std::invalid_argument("zero port in one of locations");
        if (!loc.backlog) loc.backlog = 4096;
    }

    locations = config.locations;
    conn_conf = config.conn_conf;
}

void Server::reconfigure (const ServerConfig& conf) {
    reconfigure(this, conf);
}

void Server::run () {
    if (running) throw std::logic_error("already running");
    running = true;
    panda_log_info("websocket::Server::run with conn_conf:" << conn_conf);

    start_listening();
}

ServerConnectionSP Server::new_connection (uint64_t id) {
    auto res = new ServerConnection(this, id);
    res->configure(conn_conf);
    return res;
}

void Server::on_connection (ServerConnectionSP conn) {
    connection_callback(this, conn);
}

void Server::on_remove_connection (ServerConnectionSP conn, uint16_t code, string payload) {
    disconnection_callback(this, conn, code, payload);
}

void Server::start_listening () {
    for (auto& location : locations) {
        auto l = new Listener(_loop, location);
        l->connection_event.add(std::bind(&Server::on_connect, this, _1, _2));
        l->run();
        listeners.push_back(l);
    }
}

void Server::stop_listening () {
    listeners.clear();
}

void Server::on_connect (Stream* listener, const StreamError& err) {
    if (err) {
        panda_log_info("Server[on_connect]: error: " << err.what());
        return;
    }
    if (auto l = dyn_cast<Listener*>(listener)) {
        panda_log_info("Server[on_connect]: somebody connected to " << l->location());
    }

    auto conn = new_connection(++lastid);
    connections[conn->id()] = conn;
    conn->eof_event.add(std::bind(&Server::on_disconnect, this, _1));
    conn->run(listener);

    on_connection(conn);

    panda_log_info("Server[on_connect]: now i have " << connections.size() << " connections");
}

void Server::on_disconnect (Stream* handle) {
    auto conn = dyn_cast<ServerConnection*>(handle);
    panda_log_info("Server[on_disconnect]: disconnected id " << conn->id());
    remove_connection(conn);
}

void Server::remove_connection (ServerConnectionSP conn, uint16_t code, string payload) {
    auto erased = connections.erase(conn->id());
    if (!erased) return;
    on_remove_connection(conn, code, payload);
    panda_log_info("Server[remove_connection]: now i have " << connections.size() << " connections");
}

void Server::stop () {
    if (!running) return;
    panda_log_info("stop!");
    stop_listening();
    connections.clear();
    running = false;
}

Server::~Server () {
    stop();
    panda_log_info("server destroy");
}
