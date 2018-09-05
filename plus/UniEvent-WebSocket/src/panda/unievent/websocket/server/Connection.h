#pragma once
#include "../ConnectionBase.h"
#include <panda/CallbackDispatcher.h>
#include <panda/protocol/websocket/ServerParser.h>


namespace panda { namespace unievent { namespace websocket {

struct Server;

namespace server {
    using panda::CallbackDispatcher;
    using panda::protocol::websocket::ConnectRequestSP;

    struct Connection : virtual ConnectionBase {
        using SP = iptr<Connection>;

        struct Config {
            ConnectionBase::Config base;
            size_t max_handshake_size = 0;
        };

        CallbackDispatcher<void(SP, ConnectRequestSP)> accept_callback;

        Connection (Server* server, uint64_t id);

        void configure (const Config& conf);

        uint64_t id () const { return _id; }

        virtual void run (Stream* listener);

        virtual void send_accept_error    (HTTPResponse* res);
        virtual void send_accept_response (ConnectResponse* res);

        using ConnectionBase::close;
        virtual void close (uint16_t code = uint16_t(CloseCode::DONE), string payload = string()) override;

    protected:
        virtual void on_accept (ConnectRequestSP request);

        virtual ~Connection () {
            panda_log_debug("connection destroy");
        }

    private:
        uint64_t     _id;
        Server*      _server;
        ServerParser _parser;
        bool         _alive;

        void on_read (const string& buf, const CodeError* err) override;
    };
    
    using ConnectionSP = Connection::SP;
    
    std::ostream& operator<< (std::ostream& stream, const Connection::Config& conf);
}

}}}
