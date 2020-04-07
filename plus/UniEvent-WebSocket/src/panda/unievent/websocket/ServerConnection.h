#pragma once
#include "Connection.h"
#include <panda/protocol/websocket/ServerParser.h>

namespace panda { namespace unievent { namespace websocket {

struct Listener;

using panda::protocol::websocket::ConnectRequestSP;

struct Server;

struct ServerConnection;
using ServerConnectionSP = iptr<ServerConnection>;

struct ServerConnection : virtual Connection {
    using accept_fptr = void(const ServerConnectionSP&, const ConnectRequestSP&);
    using accept_fn   = function<accept_fptr>;

    CallbackDispatcher<accept_fptr> accept_event;

    ServerConnection (Server* server, uint64_t id, const Config& conf);

    uint64_t id () const { return _id; }

    /** @param listener for derived classes to know wich listener they are on */
    virtual void run (Listener* listener);

    virtual void send_accept_error    (panda::protocol::http::Response*);
    virtual void send_accept_response (ConnectResponse*);

    template <typename T = Server> T* get_server () const { return dyn_cast<T*>(server); }

protected:
    virtual void on_accept (const ConnectRequestSP&);

    void on_read (string&, const ErrorCode&) override;

    void do_close (uint16_t code, const string& payload) override;

    ~ServerConnection () {
        panda_mlog_verbose_debug(uewslog, "connection destroy " << this);
    }

private:
    friend Server;

    uint64_t     _id;
    Server*      server;
    ServerParser parser;

    void endgame () { server = nullptr; }
};

}}}
