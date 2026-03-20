#pragma once
#include <chrono>
#include <coroutine>
#include <list>
#include <span>
#include <thread>
#include <variant>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <poll.h>
#endif

#include "cohandle.hpp"
#include "generator-pre.hpp"
#include "task-handle-pre.hpp"

namespace coop {
struct IOWaitResult;
struct SingleEvent;
struct MultiEvent;

#if defined(_WIN32)
using IOHandle = SOCKET;
#else
using IOHandle = int;
#endif

struct Running {
};

struct ByTimer {
    std::chrono::system_clock::time_point suspend_until;
};

struct BySingleEvent {
    SingleEvent* event;
};

struct ByMultiEvent {
    MultiEvent* event;
};

struct ByIO {
    IOWaitResult* result;
    IOHandle      file;
    bool          read;
    bool          write;
};

struct ByAwaiting {
};

using SuspendReason = std::variant<Running, ByTimer, BySingleEvent, ByMultiEvent, ByIO, ByAwaiting>;

constexpr auto running_index         = 0;
constexpr auto by_timer_index        = 1;
constexpr auto by_single_event_index = 2;
constexpr auto by_multi_event_index  = 3;
constexpr auto by_io_index           = 4;
constexpr auto by_awaiting_index     = 5;

struct Task {
    std::coroutine_handle<> handle;
    Task*                   parent;
    TaskHandle*             user_handle = nullptr;
    std::list<Task>         children;
    size_t                  objective_of = 0;
    SuspendReason           suspend_reason;
    bool                    handle_owned = true;
    bool                    zombie       = false;
};

struct Runner {
    // private
    using sc = std::chrono::system_clock;
    struct GatheringResult {
        std::vector<Task*>  polling_tasks;
        std::vector<pollfd> polling_fds;
        sc::time_point      now   = sc::now();
        sc::duration        sleep = sc::duration::max();
    };

    Task               root;
    Task*              current_task = &root;
    size_t             loop_count   = 0;
    std::vector<Task*> running_tasks;
    std::vector<bool>  objective_task_finished;

    // private
    auto dump_task_tree(const Task& task, int depth = 0) -> void;
    auto gather_resumable_tasks(Task& task, GatheringResult& result) -> void;
    auto run_tasks() -> void;

    // coop internal
    template <CoHandleLike CoHandle>
    auto push_task(bool independent, bool transfer_handle, CoHandle& handle, TaskHandle* user_handle, size_t objective_of) -> void;
    auto destroy_task(std::list<Task>::iterator iter) -> bool;
    auto remove_task(Task& task) -> bool;

    // for awaiters
    auto join(TaskHandle& handle) -> void;
    auto delay(std::chrono::system_clock::duration duration) -> void;
    auto event_wait(SingleEvent& event) -> void;
    auto event_notify(SingleEvent& event) -> void;
    auto event_wait(MultiEvent& event) -> void;
    auto event_notify(MultiEvent& event, size_t n) -> void;
    auto io_wait(IOHandle fd, bool read, bool write, IOWaitResult& result) -> void;

    // public
    template <CoGeneratorLike Generator>
    auto push_task(Generator generator, TaskHandle* user_handle = nullptr) -> void;
    template <CoGeneratorLike Generator>
    auto push_dependent_task(Generator generator) -> void;
    template <CoGeneratorLike Generator>
    auto await(Generator generator) -> decltype(auto);
    auto cancel_task(TaskHandle& handle) -> bool;
    auto run() -> void;
};

struct RunnerGetter {
    Runner* runner;

    auto await_ready() const -> bool {
        return false;
    }

    template <CoHandleLike CoHandle>
    auto await_suspend(CoHandle caller_task) -> void {
        runner = caller_task.promise().runner;
    }

    auto await_resume() const -> Runner* {
        return runner;
    }
};

using reveal_runner = RunnerGetter;

struct NopAwaiter {
    auto await_ready() const -> bool {
        return false;
    }

    template <CoHandleLike CoHandle>
    auto await_suspend(CoHandle) -> void {
    }

    auto await_resume() const -> void {
    }
};

using yield = NopAwaiter;
} // namespace coop
