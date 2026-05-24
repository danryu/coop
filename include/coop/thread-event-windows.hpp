#pragma once
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
    const auto sock = pipe.producer();

    auto mode = u_long();

    // set to nonblocking
    mode = 1;
    ASSERT(ioctlsocket(sock, FIONBIO, &mode) == 0, "errno={}", WSAGetLastError());

    auto count = size_t(0);
    while(true) {
        auto buffer = std::byte();
        if(pipe.read(&buffer, 1) != 1) {
            const auto error = WSAGetLastError();
            ASSERT(error == WSAEWOULDBLOCK, "errno={}", error);
            break;
        }
        count += 1;
    }

    // set to blocking
    mode = 0;
    ASSERT(ioctlsocket(sock, FIONBIO, &mode) == 0, "errno={}", WSAGetLastError());

    return count;
}

inline auto ThreadEvent::notify() -> void {
    ASSERT(pipe.write("", 1) == 1, "errno={}", WSAGetLastError());
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
