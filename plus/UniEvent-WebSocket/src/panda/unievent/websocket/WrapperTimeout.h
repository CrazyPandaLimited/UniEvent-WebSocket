#pragma once

#include <panda/unievent/Timer.h>
#include <panda/excepted.h>

namespace panda { namespace unievent { namespace websocket {

struct WrapperTimeout {
    void set(uint64_t timeout) {
        this->value = timeout;
    }

    void start(const LoopSP& loop) {
        t0 = loop->now();
        this->loop = loop;
    }

    excepted<void, ErrorCode> next(const function<void()>& cb) {
        if (value == 0) return {};
        auto dt = loop->now() - t0;
        if (value > dt) {
            timer = Timer::once(value - dt, [cb](auto){cb();}, loop);
            return {};
        } else {
            cb();
            return make_unexpected(make_error_code(std::errc::timed_out));
        }
    }

    void done() {
        if (value == 0) return;
        if (timer) {
            timer->stop();
            timer.reset();
        }
    }

    uint64_t value    = 0;
    uint64_t t0       = 0;
    LoopSP   loop;
    TimerSP  timer;
};

}}}
