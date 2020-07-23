#pragma once

#include <panda/unievent/Timer.h>
#include <queue>

namespace panda { namespace unievent { namespace websocket {

struct SharedTimeout {
    SharedTimeout(const LoopSP& loop) : loop(loop) {}

    void set(uint64_t timeout, const LoopSP& loop = {}) {
        this->value = timeout;
        if (loop) {
            this->loop = loop;
        }
        callbacks = {};
        timer.reset();
    }

    void add_step(const function<void()>& cb) {
        if (callbacks.empty()) {
            timer = Timer::once(value, [this](const TimerSP&) {
                callbacks.front()();
            }, loop);
        }
        callbacks.push(cb);
    }

    void end_step() {
        callbacks.pop();
        if (callbacks.empty()) {
            timer->stop();
            timer.reset();
        }
    }

    uint64_t value = 0;
    LoopSP   loop;
    TimerSP  timer;

    std::queue<function<void()>> callbacks;
};

}}}
