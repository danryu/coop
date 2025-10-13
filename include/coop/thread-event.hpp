#pragma once
#if defined(_WIN32)
#include "thread-event-windows.hpp"
#elif defined(__APPLE__)
#include "thread-event-macos.hpp"
#else
#include "thread-event-unix.hpp"
#endif
