#pragma once
#include "cohandle.hpp"
#include "io-pre.hpp"

// emulate eventfd with socket pipe
#if defined(_WIN32) || defined(__APPLE__)
#include "pipe.hpp"
#endif

namespace coop {
struct [[nodiscard]] ThreadEvent {
#if defined(_WIN32) || defined(__APPLE__)
    Pipe pipe;
#else
    int fd = -1;
#endif
    IOWaitResult result;

    auto await_ready() const -> bool;
    template <CoHandleLike CoHandle>
    auto await_suspend(CoHandle caller_task) -> void;
    auto await_resume() -> size_t;

    auto notify() -> void;

    ThreadEvent(ThreadEvent&& o);
    ThreadEvent();
    ~ThreadEvent();
};
} // namespace coop
