#pragma once

#include <panda/CallbackDispatcher.h>
#include <panda/websocket/ClientParser.h>
#include <panda/websocket/BaseConnection.h>


namespace panda {
namespace websocket {

using panda::websocket::ClientParser;

class Client : public virtual BaseConnection
{
public:
    Client(Loop* loop = Loop::default_loop());
    virtual ~Client();

    CallbackDispatcher<void(shared_ptr<Client, true>, ConnectResponseSP)>  connect_callback;

    /** @param port default value is 443 for secure and 80 for usual     */
    void connect(ConnectRequestSP request, bool secure = false, uint16_t port = 0);

protected:
    virtual void on_connect      (ConnectResponseSP response);

    using TCP::connect;

private:
    virtual void on_connect (const StreamError& err, event::ConnectRequest* req) override;
    virtual void on_read (const string& buf, const StreamError& err) override;

    ClientParser parser;
};

}
}
