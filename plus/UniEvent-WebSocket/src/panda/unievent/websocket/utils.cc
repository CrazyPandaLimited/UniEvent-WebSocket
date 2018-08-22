#include <panda/unievent/websocket/utils.h>

const char* panda::unievent::websocket::c_str (string& str) {
    auto len = str.length();
    auto cap = str.shared_capacity();
    if (cap <= len || str.data()[len] != 0) {
        char* buf = str.reserve(len+1);
        buf[len] = 0;
    }
    return str.data();
}
