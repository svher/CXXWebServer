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
    void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
    std::string BacktraceToString(int size = 64, const std::string& prefix = "", int skip = 2);
}