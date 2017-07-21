#include <panda/websocket/server/Server.h>

namespace panda { namespace websocket { namespace server {

using std::cout;
using namespace std::placeholders;

Server::Server (Loop* loop) : _loop(loop), lastid(0), running(false) {
    cout << "Server(): loop is default = " << (_loop == Loop::default_loop()) << "\n";
}

void Server::init (ServerConfig config) {
	if (!config.locations.size()) throw std::invalid_argument("no locations to listen supplied");

	for (auto& loc : config.locations) {
		if (!loc.host)    throw std::invalid_argument("empty host in one of locations");
		if (!loc.port)    throw std::invalid_argument("zero port in one of locations");
		if (!loc.backlog) loc.backlog = 1024;
	}

	locations = config.locations;
}

//void die (int en, const char* msg) {
//	cout << "error(" << msg << "): " << (en ? strerror(en) : "") << "\n";
//	throw "ebanarot";
//}

//void kick_client (Stream* handle) {
//	handle->write("you are kicked\n", 15);
//	handle->disconnect();
//	clients.erase(std::find(clients.begin(), clients.end(), handle));
//	//cout <<"Thread("<<tnum<<") client kicked, now i have " << clients.size() << " clients\n";
//}

//void on_timer (Timer* timer) {
//	//cout <<"Thread("<<tnum<<") timer\n";
//}

void Server::run () {
	if (running) throw std::logic_error("already running");
	running = true;
	cout << "run\n";

	for (auto& location : locations) {
		auto l = new Listener(_loop, location);
        l->connection_callback = std::bind(&Server::on_connect, this, _1, _2);
		l->run();
		listeners.push_back(l);
	}
}

Connection* Server::new_connection (uint64_t id) {
    return new Connection(this, id);
}

void Server::on_connection(Connection* conn) {
    if (connection_callback) connection_callback(this, conn);
}

void Server::on_remove_connection(Connection* conn) {
    if (remove_connection_callback) remove_connection_callback(this, conn);
}

void Server::on_connect (Stream* listener, const StreamError& err) {
    if (err) {
        cout << "Server[on_connect]: error: " << err.what() << "\n";
        return;
    }
    cout << "Server[on_connect]: somebody connected to " << (uint64_t)listener << "\n";

    auto conn = new_connection(++lastid);
    connections[conn->id()] = conn;
    conn->eof_callback = std::bind(&Server::on_disconnect, this, _1);
    conn->run(listener);

    on_connection(conn);

    cout << "Server[on_connect]: now i have " << connections.size() << " connections\n";
}

void Server::on_disconnect (Stream* handle) {
    auto conn = dyn_cast<Connection*>(handle);
    cout << "Server[on_disconnect]: disconnected id " << conn->id() << "\n";
    remove_connection(conn);
}

void Server::remove_connection (Connection* conn) {
    connections.erase(conn->id());
    on_remove_connection(conn);

    cout << "Server[remove_connection]: now i have " << connections.size() << " connections\n";
}

void Server::stop () {
	if (!running) return;
	cout << "stop!\n";
	listeners.clear();
	connections.clear();
	running = false;
}

Server::~Server () {
    stop();
    cout << "server destroy\n";
}

}}}
