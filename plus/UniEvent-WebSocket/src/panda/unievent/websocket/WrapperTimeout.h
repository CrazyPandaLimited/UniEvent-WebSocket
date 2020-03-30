#pragma once

#include <panda/unievent/Timer.h>

namespace panda { namespace unievent { namespace websocket {

struct WrapperTimeout {
    void set(uint64_t timeout) {
        this->value = timeout;
    }

    void start(const LoopSP& loop) {
        t0 = loop->now();
        this->loop = loop;
    }

    void next(const function<void()>& cb) {
        if (value == 0) return;
        auto dt = loop->now() - t0;
        if (value > dt) {
            timer = Timer::once(value - dt, [cb](auto){cb();}, loop);
        } else {
            delay_id = loop->delay(cb);
        }
    }

    void done() {
        if (value == 0) return;
        if (delay_id) {
            loop->cancel_delay(delay_id);
        }
        if (timer) {
            timer->stop();
        }
    }

    uint64_t value    = 0;
    uint64_t t0       = 0;
    uint64_t delay_id = 0;
    LoopSP   loop;
    TimerSP  timer;
};

}}}
