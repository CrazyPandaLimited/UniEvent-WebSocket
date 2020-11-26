#include <catch2/catch.hpp>
#include <panda/log.h>
#include <panda/unievent/test/AsyncTest.h>
#include <panda/unievent/websocket/Client.h>
#include <panda/unievent/websocket/Server.h>
#include <panda/unievent/websocket/SharedTimeout.h>
#include <thread>

#define panda_log_module panda::unievent::websocket::panda_log_module

using namespace panda::unievent::websocket;
using panda::unievent::test::AsyncTest;
using panda::unievent::LoopSP;
using panda::unievent::StreamSP;
using panda::string;

static bool _init () {
    //panda::log::set_level(panda::log::VerboseDebug);
    panda::log::set_level(panda::log::WARNING);
    panda::log::set_logger([](auto& msg, auto&) {
        WARN("" << msg);
    });
    return true;
}
static bool __init = _init();

struct Pair {
    ServerSP server;
    ClientSP client;
};

static ServerSP make_server (LoopSP loop, uint16_t& port) {
    ServerSP server = new Server(loop);
    Server::Config conf;
    Location loc;
    loc.host = "127.0.0.1";
    conf.locations.push_back(loc);
    server->configure(conf);
    server->run();
    port = server->get_listeners()[0]->sockaddr().port();
    return server;
}

static Pair make_pair (LoopSP loop) {
    uint16_t port;
    ServerSP server = make_server(loop, port);
    ClientSP client = new Client(loop);
    ClientConnectRequestSP req = new ClientConnectRequest();
    req->uri = new URI();
    req->uri->host("127.0.0.1");
    req->uri->scheme("ws");
    req->uri->port(port);
    client->connect(req);
    return {server, client};
}

TEST_CASE("send ranges compiles", "[uews]") {
    if (false) {
        ServerConnectionSP conn;

        string msgs[] = {"1", "2", "3"};
        auto rr = msgs | ::ranges::view::transform([](string& s) -> string& {
            return s;
        });
        conn->send_message(begin(rr), end(rr));
    }
}

TEST_CASE("on_read after close", "[uews]") {
    AsyncTest test(1000, {"connect", "close"});
    {
        auto p = make_pair(test.loop);

        ServerConnectionSP sconn;
        p.server->connection_event.add([&](auto, auto conn) {
            sconn = conn;
            sconn->accept_event.add([](auto conn, auto) {
                string msg = "123";
                conn->send_message(msg);

            });
        });
        test.await(p.client->message_event, "connect");

        const size_t SIZE = 120 * 1024;
        string big_buf;
        big_buf.resize(SIZE);
        for (size_t i = 0; i < SIZE; ++i) {
            big_buf[i] = '1';
        }

        sconn->write(big_buf);

        size_t rcount = 0;
        p.client->read_event.add([&](auto client, auto&, auto& err){
            if (err) WARN(err);
            REQUIRE_FALSE(err);
            rcount++;
            client->shutdown();
            client->disconnect();
        });

        test.await(sconn->eof_event, "close");
        REQUIRE(rcount == 1);
    }

    test.run();
}

TEST_CASE("destroying server&client in callbacks", "[uews]") {
    AsyncTest test(1000, 1);
    {
        auto p = make_pair(test.loop);
        auto srm = [&](auto...) {
            test.happens();
            CHECK(true);
            p.server->stop();
            p.server = nullptr;
        };
        auto crm = [&](auto...){
            test.happens();
            CHECK(true);
            p.client = nullptr;
            p.server->stop_listening();
        };

        SECTION("server - connection_event")    { p.server->connection_event.add(srm); }
        SECTION("server - disconnection_event") { p.server->disconnection_event.add(srm); }

        p.server->connection_event.add([&](auto, auto conn) {
            panda_log_info("s-conn");
            SECTION("server - accept_event")     { conn->accept_event.add(srm); }
            SECTION("server - message_event")    { conn->message_event.add(srm); }
            SECTION("server - close_event")      { conn->close_event.add(srm); }
            SECTION("server - peer_close_event") { conn->peer_close_event.add(srm); }
            SECTION("server - ping_event")       { conn->ping_event.add(srm); }
            SECTION("server - pong_event")       { conn->pong_event.add(srm); }

            conn->message_event.add([&](auto conn, auto) {
                conn->send_ping();
            });
            conn->ping_event.add([&](auto conn, auto) {
                string omsg = "epta";
                conn->send_message(omsg);
            });

            conn->error_event.add([&](auto, auto& err) { panda_log_info("s-err: " << err); });
            conn->accept_event.add([&](auto...)        { panda_log_info("s-accept"); });
            conn->message_event.add([&](auto...)       { panda_log_info("s-message"); });
            conn->close_event.add([&](auto...)         { panda_log_info("s-close"); });
            conn->peer_close_event.add([&](auto...)    { panda_log_info("s-peer-close"); });
            conn->ping_event.add([&](auto...)          { panda_log_info("s-ping"); });
            conn->pong_event.add([&](auto...)          { panda_log_info("s-pong"); });
        });

        bool ce = true;
        SECTION("client - connect_event") { p.client->connect_event.add(crm); ce = false; }

        if (ce) p.client->connect_event.add([&](auto client, auto res) {
            panda_log_info("c-conn");
            SECTION("client - message_event")    { client->message_event.add(crm); }
            SECTION("client - close_event")      { client->close_event.add(crm); }
            SECTION("client - ping_event")       { client->ping_event.add(crm); }
            SECTION("client - pong_event")       { client->pong_event.add(crm); }

            if (res->error) return;

            string omsg = "nah";
            client->send_message(omsg);

            client->ping_event.add([&](auto client, auto) {
                client->send_ping();
            });
            client->message_event.add([&](auto client, auto) {
                client->close();
            });

            client->error_event.add([&](auto, auto& err) { panda_log_info("c-err: " << err); });
            client->message_event.add([&](auto...)       { panda_log_info("c-message"); });
            client->close_event.add([&](auto...)         { panda_log_info("c-close"); });
            client->peer_close_event.add([&](auto...)    { panda_log_info("c-peer-close"); });
            client->ping_event.add([&](auto...)          { panda_log_info("c-ping"); });
            client->pong_event.add([&](auto...)          { panda_log_info("c-pong"); });
        });

        test.run();
    }

    test.run(); // hang check
}

TEST_CASE("destroying server in error callback", "[uews]") {
    AsyncTest test(1000, 1);
    auto p = make_pair(test.loop);

    p.server->connection_event.add([&](auto, auto conn) {
        conn->error_event.add([&](auto, auto&){
            test.happens();
            CHECK(true);
            //WARN("err = " << err.what());
            p.server->stop();
            p.server = nullptr;
        });
    });

    p.client->connect_event.add([&](auto client, auto) {
        client->write("fuck you bitch");
    });

    test.run();
    test.run(); // hang check
}

TEST_CASE("destroying client in error callback", "[uews]") {
    AsyncTest test(1000, 1);
    auto p = make_pair(test.loop);

    p.server->connection_event.add([&](auto, auto conn) {
        conn->accept_event.add([&](auto conn, auto) {
            conn->write("f1uck you dudefuck you dudefuck you dudefuck you dudefuck you dudefuck you dudefuck you dudefuck you");
        });
    });

    p.client->error_event.add([&](auto, auto&){
        test.happens();
        CHECK(true);
        //WARN("err = " << err.what());
        p.server->stop_listening();
        p.client = nullptr;
    });

    test.run();
    test.run(); // hang check
}

TEST_CASE("cleanup on success", "[uews]") {
    AsyncTest test(1000, {"connect", "srecv", "crecv", "disconn"});
    {
        auto p = make_pair(test.loop);
        p.server->connection_event.add([&](auto, auto conn) {
            conn->message_event.add([&](auto conn, auto msg) {
                test.happens("srecv");
                CHECK(msg->payload[0] == "nah");
                string omsg = "epta";
                conn->send_message(omsg);
            });
            conn->peer_close_event.add([&](auto...) {
                test.happens("disconn");
                SECTION("stop everything") {
                    p.server->stop();
                }
                SECTION("stop listening") {
                    p.server->stop_listening();
                }
                SECTION("stop loop") {
                    test.loop->stop();
                }
            });
        });

        p.client->connect_event.add([&](auto client, auto res) {
            CHECK(!res->error);
            test.happens("connect");
            string omsg = "nah";
            client->send_message(omsg);

            client->message_event.add([&](auto client, auto msg) {
                test.happens("crecv");
                CHECK(msg->payload[0] == "epta");
                client->close();
            });
        });

        test.run();
    }

    test.run(); // everything should be destroyed and not holding the loop
}

TEST_CASE("connect and close", "[uews]") {
    AsyncTest test(1000, 0);
    auto p = make_pair(test.loop);
    p.client->close();
    p.server = nullptr;
    test.run();
    SUCCEED("ok");
}

TEST_CASE("connect timeout", "[uews]") {
    AsyncTest test(1000, {"connect"});
    using panda::net::SockAddr;
    using panda::uri::URI;

    panda::iptr<panda::Refcnt> server;
    panda::unievent::StreamSP sconn;
    bool error = true;
    const uint64_t TIME = 5;

    SockAddr addr;
    ClientSP client = new Client(test.loop);

    SECTION("blackhole") {
        addr = test.get_blackhole_addr();
    }
    SECTION("no ws") {
        panda::unievent::TcpSP tserver = new panda::unievent::Tcp(test.loop);
        tserver->bind("127.0.0.1", 0);
        tserver->listen(1);
        tserver->connection_event.add([&](auto, auto conn, auto) {
            sconn = conn;
        });
        addr = tserver->sockaddr();
        server = tserver;
    }
    SECTION("real") {
        error = false;
        uint16_t port = 0;
        auto wsserver = make_server(test.loop, port);
        addr = SockAddr::Inet4("127.0.0.1", port);
        test.expected.push_back("cb");
        client->connect_event.add([&](auto, auto) {
            test.happens("cb"); // to check it called once. Checking error is wrong in cases of slow connect or under stress
        });
        server = wsserver;
    }


    ClientConnectRequestSP req = new ClientConnectRequest();
    req->uri = new URI;
    req->uri->host(addr.ip());
    req->uri->port(addr.port());
    req->uri->scheme(panda::unievent::websocket::ws_scheme(false));
    req->timeout.set(TIME);

    client->connect(req);

    auto tup = test.await(client->connect_event, "connect");
    if (error) {
        REQUIRE(std::get<1>(tup)->error.contains(make_error_code(std::errc::timed_out)));
    } else {
        test.wait(TIME + 1);
    }
}

TEST_CASE("last ref in connect timeout", "[errors]") {
    AsyncTest test(1000, {"timeout"});

    StreamSP sconn;
    panda::unievent::TcpSP tserver = new panda::unievent::Tcp(test.loop);
    tserver->bind("127.0.0.1", 0);
    tserver->listen(1);
    tserver->connection_event.add([&](auto, auto conn, auto) {
        sconn = conn;
    });
    auto addr = tserver->sockaddr();

    ClientSP client = new Client(test.loop);

    ClientConnectRequestSP req = new ClientConnectRequest();
    req->uri = new panda::uri::URI;
    req->uri->host(addr.ip());
    req->uri->port(addr.port());
    req->uri->scheme(panda::unievent::websocket::ws_scheme(false));
    req->timeout.set(10);
    client->connect(req);

    client->connect_event.add([&](const auto&...) {
        client.reset();
        test.happens("timeout");
        test.loop->stop();
    });

    test.run();
}

TEST_CASE("simple connect", "[errors]") {
    AsyncTest test(1000, {"connected"});

    uint16_t port = 0;
    auto server = make_server(test.loop, port);

    ClientSP client = new Client(test.loop);
    client->connect("127.0.0.1/path/abc", false, port);

    test.await(client->connect_event, "connected");
}

TEST_CASE("no close frame on_eof", "[errors]") {
    AsyncTest test(1000, {"connected", "eof", "close"});

    uint16_t port = 0;
    auto server = make_server(test.loop, port);
    ClientSP client = new Client(test.loop);
    client->connect("127.0.0.1", false, port);
    test.await(client->connect_event, "connected");

    client->shutdown(); // emulates broken connection

    client->close_event.add([&](auto...) {
        test.happens("close");
    });

    test.await(client->eof_event, "eof"); // close should be emulated with 1006 after eof received, if server answer it happens before
}

TEST_CASE("shutdown timeout", "[uews]") {
    AsyncTest test(1000, {"shutdown"});
    auto p = make_pair(test.loop);

    test.await(p.client->connect_event);
    Connection::Config conf;
    conf.shutdown_timeout = 1;
    p.client->configure(conf);

    p.client->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    {
        auto tup = test.await(p.client->shutdown_event, "shutdown");
        REQUIRE(std::get<1>(tup) & std::errc::timed_out);
    }
    REQUIRE(p.client->refcnt() == 1);
}

TEST_CASE("SharedTimeout", "[uews]") {
    AsyncTest test(1000, {});
    SharedTimeout t(test.loop);
    t.set(10);
    SECTION("single") {
        t.add_step([&]{test.happens("timeout");});
        SECTION("timeout") {
            test.expected.push_back("timeout");
        }
        SECTION("stopped") {
            t.end_step();
        }
        test.wait(15);
    }
    SECTION("multi") {
        t.add_step([&]{test.happens("timeout1");});
        t.add_step([&]{test.happens("timeout2");});
        SECTION("timeout") {
            test.expected.push_back("timeout1");
        }
        SECTION("timeout2") {
            test.expected.push_back("timeout2");
            t.end_step();
        }
        SECTION("stopped") {
            t.end_step();
            t.end_step();
        }
        test.wait(15);
    }
}
