#include "webserver.h"

svher::Logger::ptr g_logger = LOG_ROOT();

void test_fiber(int arg) {
    static int cnt = 5;
    LOG_INFO(g_logger) << "test fiber, arg: " << arg << " cnt: " << cnt;
    sleep(1);
    if (cnt--)
        svher::Scheduler::GetThis()->schedule( std::bind(&test_fiber, arg));
    sleep(1);

}

int main() {
    svher::Scheduler sc(5, true, "main");
    sc.start();
    sc.schedule(std::bind(&test_fiber, 2));
    sc.stop();
}