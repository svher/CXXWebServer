#include "util.h"
#include "fiber.h"
#include <execinfo.h>

namespace svher {
    static Logger::ptr g_logger = LOG_NAME("sys");

    pid_t GetThreadId() {
        return syscall(SYS_gettid);
    }

    uint32_t GetFiberId() {
        return Fiber::GetFiberId();
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip) {
        void** array = (void**)malloc((sizeof(void*) * size));
        size_t s = ::backtrace(array, size);
        char** strings = backtrace_symbols(array, s);
        if (strings == NULL) {
            LOG_ERROR(g_logger) << "backtrack_symbols error";
            return;
        }
        for (size_t i = skip; i < s; ++i) {
            bt.push_back(strings[i]);
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, const std::string& prefix, int skip) {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i) {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

}