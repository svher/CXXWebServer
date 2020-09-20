#pragma once

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include "log.h"

namespace svher {
    pid_t GetThreadId();
    uint32_t GetFiberId();
    void Backtrace(std::vector<std::string>& bt, int size, int skip);
    std::string BacktraceToString(int size, const std::string& prefix = "", int skip = 2);
}