#pragma once
#include <fcntl.h>
#include <unistd.h>

#include "runner.hpp"
#include "thread-event-pre.hpp"

#include "assert-def.hpp"

namespace coop {
inline auto ThreadEvent::await_ready() const -> bool {
    return false;
}

template <CoHandleLike CoHandle>
inline auto ThreadEvent::await_suspend(CoHandle caller_task) -> void {
    const auto runner = caller_task.promise().runner;
    runner->io_wait(pipe.producer(), true, false, result);
}

inline auto ThreadEvent::await_resume() -> size_t {
    const auto fd = pipe.producer();

    // set to nonblocking
    const auto flags = fcntl(fd, F_GETFL, 0);
    ASSERT(flags != -1, "fcntl F_GETFL failed, errno={}", errno);
    ASSERT(fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1, "fcntl F_SETFL failed, errno={}", errno);

    auto count = size_t(0);
    while(true) {
        auto buffer = std::byte();
        const auto r = pipe.read(&buffer, 1);
        if(r != 1) {
            ASSERT(r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK), "read failed, errno={}", errno);
            break;
        }
        count += 1;
    }

    // set back to blocking
    ASSERT(fcntl(fd, F_SETFL, flags) != -1, "fcntl F_SETFL failed, errno={}", errno);

    return count;
}

inline auto ThreadEvent::notify() -> void {
    ASSERT(pipe.write("", 1) == 1, "write failed, errno={}", errno);
}

inline ThreadEvent::ThreadEvent(ThreadEvent&& o)
    : pipe(std::move(o.pipe)) {
}

inline ThreadEvent::ThreadEvent() {
}

inline ThreadEvent::~ThreadEvent() {
}
} // namespace coop

#include "assert-undef.hpp"
