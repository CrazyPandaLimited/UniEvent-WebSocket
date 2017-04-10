#include <panda/websocket/server/utils.h>

namespace panda { namespace websocket { namespace server {

const char* c_str (string& str) {
    auto len = str.length();
    auto cap = str.shared_capacity();
    if (cap <= len || str.data()[len] != 0) {
        char* buf = str.reserve(len+1);
        buf[len] = 0;
    }
    return str.data();
}

}}}
