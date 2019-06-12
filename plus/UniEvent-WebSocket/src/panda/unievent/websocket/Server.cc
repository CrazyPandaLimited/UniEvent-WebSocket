#include "Server.h"
#include <panda/log.h>

using namespace std::placeholders;
using namespace panda::unievent::websocket;

std::atomic<uint64_t> Server::lastid(0);

Server::Server (const LoopSP& loop) : running(false), _loop(loop) {
    panda_log_info("Server(): loop is default = " << (_loop == Loop::default_loop()));
}

void Server::on_delete () noexcept {
    try {
        stop();
    }
    catch (const std::exception& e) {
        panda_log_critical("[Websocket ~Server] exception caught in while stopping server: " << e.what());
    }
    catch (...) {
        panda_log_critical("[Websocket ~Server] unknown exception caught while stopping server");
    }
    panda_log_info("server destroy");
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
        if (!loc.backlog) throw std::invalid_argument("zero backlog in one of locations");
        //do not check port, 0 is some free port and you can get if with get_listeners()[i]->sockadr();
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

void Server::stop (uint16_t code) {
    if (!running) return;
    running = false;
    panda_log_info("stop!");
    stop_listening();

    auto tmp = connections;
    for (auto& it : tmp) {
        auto& conn = it.second;
        conn->close(code);
        conn->endgame();
        conn.reset();
    }
    // connections may not be empty if stop() is called from on_close() callback
}

void Server::start_listening () {
    for (auto& location : locations) {
        auto l = new Listener(_loop, location);
        l->connection_event.add(std::bind(&Server::on_tcp_connection, this, _1, _2, _3));
        l->connection_factory = [this]() { return new_connection(++lastid); };
        l->run();
        listeners.push_back(l);
    }
}

void Server::stop_listening () {
    listeners.clear();
}

ServerConnectionSP Server::new_connection (uint64_t id) {
    return new ServerConnection(this, id, conn_conf);
}

void Server::on_tcp_connection (const StreamSP& _lstn, const StreamSP& _conn, const CodeError& err) {
    if (err) {
        panda_log_info("Server[on_tcp_connection]: error: " << err.whats());
        return;
    }

    auto connection = dynamic_pointer_cast<ServerConnection>(_conn);
    connections[connection->id()] = connection;
    connection->run();

    auto listener = dynamic_pointer_cast<Listener>(_lstn);
    panda_log_info("Server[on_tcp_connection]: somebody connected to " << listener->location() << ", now i have " << connections.size() << " connections");

    on_connection(connection);
}

void Server::on_connection (const ServerConnectionSP& conn) {
    connection_event(this, conn);
}

void Server::remove_connection (const ServerConnectionSP& conn, uint16_t code, const string& payload) {
    auto erased = connections.erase(conn->id());
    assert(erased);
    panda_log_info("Server[remove_connection]: now i have " << connections.size() << " connections");
    on_disconnection(conn, code, payload);
}

void Server::on_disconnection (const ServerConnectionSP& conn, uint16_t code, const string& payload) {
    disconnection_event(this, conn, code, payload);
}
