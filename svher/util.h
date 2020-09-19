#pragma once

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

namespace svher {

    pid_t GetThreadId();
    uint32_t GetFiberId();

}