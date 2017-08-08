#pragma once

#include <panda/CallbackDispatcher.h>
#include <panda/websocket/ClientParser.h>
#include <panda/websocket/BaseConnection.h>


namespace panda {
namespace websocket {

using panda::websocket::ClientParser;

class Client : public BaseConnection/*, public panda::lib::AllocatedObject<Client>*/
{
public:
    Client(Loop* loop = Loop::default_loop());
    virtual ~Client();

    CallbackDispatcher<void(Client*, ConnectResponseSP)>  connect_callback;

    /** @param port default value is 443 for secure and 80 for usual     */
    void connect(ConnectRequestSP request, bool secure = false, uint16_t port = 0);

//    using AllocatedObject<Client>::operator new;
//    using AllocatedObject<Client>::operator delete;
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
