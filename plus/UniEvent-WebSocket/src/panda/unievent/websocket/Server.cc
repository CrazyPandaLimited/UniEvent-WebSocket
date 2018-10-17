#include "Server.h"
#include <panda/log.h>

using namespace std::placeholders;
using namespace panda::unievent::websocket;

std::atomic<uint64_t> Server::lastid(0);

Server::Server (Loop* loop) : running(false), _loop(loop) {
    panda_log_info("Server(): loop is default = " << (_loop == Loop::default_loop()));
}

void Server::configure (const Config& conf) {
    config_validate(conf);
    bool was_running = running;
    if (was_running) stop_listening();
    config_apply(conf);
    if (was_running) start_listening();
}

void Server::config_validate (const Config& c) const {
    if (!c.locations.size()) throw std::invalid_argument("no locations to listen supplied");

    for (auto& loc : c.locations) {
        if (!loc.host)    throw std::invalid_argument("empty host in one of locations");
        if (!loc.port)    throw std::invalid_argument("zero port in one of locations");
        if (!loc.backlog) throw std::invalid_argument("zero backlog in one of locations");
    }
}

void Server::config_apply (const Config& conf) {
    locations = conf.locations;
    conn_conf = conf.connection;
}

void Server::run () {
    if (running) throw std::logic_error("already running");
    running = true;
    panda_log_info("websocket::Server::run with conn_conf:" << conn_conf);

    start_listening();
}

ServerConnectionSP Server::new_connection (uint64_t id) {
    return new ServerConnection(this, id, conn_conf);
}

void Server::on_connection (ServerConnectionSP conn) {
    connection_event(this, conn);
}

void Server::on_remove_connection (ServerConnectionSP conn, uint16_t code, const string& payload) {
    disconnection_event(this, conn, code, payload);
}

void Server::start_listening () {
    for (auto& location : locations) {
        auto l = new Listener(_loop, location);
        l->connection_event.add(std::bind(&Server::on_connect, this, _1, _2, _3));
        l->connection_factory = [this]() { return new_connection(++lastid); };
        l->run();
        listeners.push_back(l);
    }
}

void Server::stop_listening () {
    listeners.clear();
}

void Server::on_connect (Stream* parent, Stream* stream, const CodeError* err) {
    if (err) {
        panda_log_info("Server[on_connect]: error: " << err->whats());
        return;
    }

    if (auto listener = dyn_cast<Listener*>(parent)) {
        panda_log_info("Server[on_connect]: somebody connected to " << listener->location());
    }

    auto connection = dyn_cast<ServerConnection*>(stream);

    connections[connection->id()] = connection;
    connection->eof_event.add(std::bind(&Server::on_disconnect, this, _1));
    connection->run();

    on_connection(connection);

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
