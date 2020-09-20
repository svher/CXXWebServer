#include "svher.h"

svher::Logger::ptr g_logger = LOG_ROOT();

int main() {
    svher::Scheduler sc;
    sc.start();
    sc.stop();
}