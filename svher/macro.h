#pragma once

#include <unistd.h>
#include <cassert>

#define ASSERT(x) \
    do \
        if (!(x)) {   \
            LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x \
                << "\nbacktrace:\n" \
                << svher::BacktraceToString(100, "   "); \
                assert(x); \
    } while (false)

#define ASSERT2(x, w) \
    do \
        if (!(x)) {   \
            LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x \
                << "\n" << w << "\n" \
                << "\nbacktrace:\n" \
                << svher::BacktraceToString(100, "   "); \
                assert(x); \
    } while (false)
