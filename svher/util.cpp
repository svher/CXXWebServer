#include "util.h"

namespace svher {
    pid_t GetThreadId() {
        return syscall(SYS_gettid);
    }

    uint32_t GetFiberId() {
        return 0;
    }

}