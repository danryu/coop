#pragma once
#include <atomic>
#include <utility>

#include <winsock2.h>

#include "assert-def.hpp"

namespace coop {
inline auto wsa_refcount = std::atomic_size_t(0);

inline auto ref_wsa() -> void {
    if(wsa_refcount.fetch_add(1) == 0) {
        auto wsadata = WSADATA();
        auto ret     = WSAStartup(WINSOCK_VERSION, &wsadata);
        ASSERT(ret == 0, "errno={}", ret);
    }
}

inline auto unref_wsa() -> void {
    if(wsa_refcount.fetch_sub(1) == 1) {
        ASSERT(WSACleanup() == 0);
    }
}

struct Pipe {
    SOCKET   fd = INVALID_SOCKET;
    uint16_t port;

    static auto build_sockaddr(uint16_t port) -> sockaddr_in {
        auto addr            = sockaddr_in();
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port        = port;
        return addr;
    }

    auto producer() -> SOCKET {
        return fd;
    }

    auto read(void* const buf, const size_t size) -> int {
        return recvfrom(fd, (char*)buf, size, 0, NULL, NULL);
    }

    auto write(const void* buf, size_t size) -> int {
        const auto fd     = socket(AF_INET, SOCK_DGRAM, 0);
        const auto addr   = build_sockaddr(port);
        const auto result = sendto(fd, (char*)buf, size, 0, (sockaddr*)(&addr), sizeof(addr));
        ASSERT(closesocket(fd) == 0, "errno={}", WSAGetLastError());
        return result;
    }

    Pipe(Pipe&& o)
        : fd(std::exchange(o.fd, INVALID_SOCKET)) {
    }

    Pipe() {
        ref_wsa();

        fd = socket(AF_INET, SOCK_DGRAM, 0);

        auto addr = build_sockaddr(0);
        ASSERT(bind(fd, (sockaddr*)(&addr), sizeof(addr)) == 0, "errno={}", WSAGetLastError());

        auto sock_name = sockaddr_in();
        auto sock_len  = int(sizeof(sock_name));
        ASSERT(getsockname(fd, (sockaddr*)(&sock_name), &sock_len) == 0, "errno={}", WSAGetLastError());
        ASSERT(sock_name.sin_addr.s_addr == htonl(INADDR_LOOPBACK), "not a loopback address addr={}", sock_name.sin_addr.s_addr);
        port = sock_name.sin_port;
    }

    ~Pipe() {
        if(fd != INVALID_SOCKET) {
            ASSERT(closesocket(fd) == 0, "errno={}", WSAGetLastError());
            fd = INVALID_SOCKET;
            unref_wsa();
        }
    }
};
} // namespace coop

#include "assert-undef.hpp"
