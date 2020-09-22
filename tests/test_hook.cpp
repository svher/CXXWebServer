#include "svher.h"

svher::Logger::ptr g_logger = LOG_ROOT();

void test_sleep(){
    svher::IOManager ioManager;
    ioManager.schedule([]() {
        sleep(2);
        LOG_INFO(g_logger) << "sleep 2";
    });

    ioManager.schedule([]() {
        sleep(3);
        LOG_INFO(g_logger) << "sleep 3";
    });
}

int main(int argc, char** argv) {
    test_sleep();
    return 0;
}