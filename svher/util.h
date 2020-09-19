#pragma once

#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <cstdio>
#include <cstdint>

namespace svher {

    pid_t GetThreadId();
    uint32_t GetFiberId();
}