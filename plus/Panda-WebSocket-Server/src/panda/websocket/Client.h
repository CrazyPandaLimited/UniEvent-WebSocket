#pragma once

#include <panda/CallbackDispatcher.h>
#include <panda/websocket/ClientParser.h>
#include <panda/websocket/BaseConnection.h>


namespace panda { namespace websocket {

using panda::websocket::ClientParser;

struct Client : virtual BaseConnection {
    Client(Loop* loop = Loop::default_loop());

    CallbackDispatcher<void(iptr<Client>, ConnectResponseSP)>  connect_callback;

    /** @param port default value is 443 for secure and 80 for usual     */
    void connect(ConnectRequestSP request, bool secure = false, uint16_t port = 0);

    virtual void close(uint16_t code = uint16_t(CloseCode::DONE), string = string()) override;

protected:
    virtual void on_stream_error (const StreamError& err) override;
    virtual void on_connect      (ConnectResponseSP response);

    using TCP::connect;

    virtual ~Client() {}

private:
    virtual void on_connect (const StreamError& err, event::ConnectRequest* req) override;
    virtual void on_read (const string& buf, const StreamError& err) override;

    ClientParser parser;
};

using ClientSP = iptr<Client>;

}}
