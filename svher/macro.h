#pragma once

#include <unistd.h>
#include <cassert>

#if defined __GNUC__ || defined __llvm__
#define LIKELY_EXECUTED(x) __builtin_expect(!!(x), 1)
#define UNLIKELY_EXECUTED(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY_EXECUTED(x) (x)
#define UNLIKELY_EXECUTED(x) (x)
#endif

#define ASSERT(x) \
    do \
        if (UNLIKELY_EXECUTED(!(x))) {   \
            LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x \
                << "\nbacktrace:\n" \
                << svher::BacktraceToString(100, "   "); \
                assert(x); \
    } while (false)

#define ASSERT2(x, w) \
    do \
        if (UNLIKELY_EXECUTED(!(x))) {   \
            LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x \
                << "\n" << w << "\n" \
                << "\nbacktrace:\n" \
                << svher::BacktraceToString(100, "   "); \
                assert(x); \
    } while (false)
