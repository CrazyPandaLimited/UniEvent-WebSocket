#include <catch.hpp>
#include <panda/unievent/test/AsyncTest.h>

#include <panda/unievent/websocket/Client.h>
#include <panda/unievent/websocket/Server.h>

using namespace panda::unievent::websocket;
using panda::unievent::test::AsyncTest;
using panda::unievent::LoopSP;
using panda::unievent::StreamSP;
using panda::string;

static ServerSP make_server(LoopSP loop, uint16_t& port) {
    ServerSP server = new Server(loop);
    Server::Config conf;
    Location loc;
    loc.host = "localhost";
    conf.locations.push_back(loc);
    server->configure(conf);
    server->run();
    port = server->get_listeners()[0]->sockaddr().port();
    return server;
}

TEST_CASE("on_read after close", "[uews]") {
    AsyncTest test(5000, {"connect", "close"});
    uint16_t port;

    ServerSP server = make_server(test.loop, port);
    ClientSP client = new Client(test.loop);

    ConnectRequestSP req = new ConnectRequest();
    req->uri = new URI();
    req->uri->host("localhost");

    client->connect(req, false, port);

    ServerConnectionSP sconn;
    server->connection_event.add([&](ServerSP, ServerConnectionSP conn) {
        sconn = conn;
        sconn->accept_event.add([](ServerConnectionSP conn, ConnectRequestSP) {
            string msg = "123";
            conn->send_message(msg);
        });
    });
    test.await(client->message_event, "connect");

    const size_t SIZE = 120 * 1024;
    string big_buf;
    big_buf.resize(SIZE);
    for (size_t i = 0; i < SIZE; ++i) {
        big_buf[i] = '1';
    }

    sconn->write(big_buf);

    size_t rcount = 0;
    client->read_event.add([&](auto, auto&, auto& err){
        REQUIRE_FALSE(err);
        rcount++;
        client->shutdown();
        client->disconnect();
    });

    test.await(sconn->eof_event, "close");
    REQUIRE(rcount == 1);
}
