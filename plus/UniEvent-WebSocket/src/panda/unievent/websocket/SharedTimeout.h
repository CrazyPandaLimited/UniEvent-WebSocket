#pragma once

#include <panda/unievent/Timer.h>
#include <queue>

namespace panda { namespace unievent { namespace websocket {

struct SharedTimeout {
    SharedTimeout(const LoopSP& loop = nullptr) : loop(loop) {}

    void set(uint64_t timeout) {
        this->value = timeout;
        reset();
    }

    void reset() {
        callbacks = {};
        timer.reset();
    }

    void add_step(const function<void()>& cb) {
        if (value == 0) return;
        if (callbacks.empty()) {
            timer = Timer::once(value, [this](const TimerSP&) {
                callbacks.front()();
            }, loop);
        }
        callbacks.push(cb);
    }

    void end_step() {
        if (value == 0) return;
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
