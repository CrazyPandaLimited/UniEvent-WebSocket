#pragma once

#include <panda/unievent/Timer.h>
#include <panda/excepted.h>
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
            start_timer(value);
        }
        callbacks.push(cb);
    }

    void end_step() {
        if (value == 0) return;
        callbacks.pop();
        if (callbacks.empty() && timer) {
            timer->stop();
            timer.reset();
        }
    }

    void add_prestep(const function<void()>& cb) {
        assert(callbacks.empty());
        t0 = loop->now();
        callbacks.push(cb);
    }

    excepted<void, ErrorCode> end_prestep() {
        assert(!callbacks.empty());
        assert(t0);
        if (value == 0) return {};
        auto dt = loop->now() - t0;
        if (value > dt) {
            start_timer(value - dt);
            return {};
        } else {
            return make_unexpected(make_error_code(std::errc::timed_out));
        }
    }

    uint64_t value = 0;
    uint64_t t0 = 0;
    LoopSP   loop;
    TimerSP  timer;

    std::queue<function<void()>> callbacks;

protected:
    void start_timer(uint64_t t) {
        timer = Timer::once(t, [this](const TimerSP&) {
            callbacks.front()();
        }, loop);
    }
};

}}}
