#include "svher.h"

svher::Logger::ptr g_logger = LOG_ROOT();

void test_fiber() {
    LOG_INFO(g_logger) << "test fiber";
}

int main() {
    svher::Scheduler sc(1, true, "main");
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();
}