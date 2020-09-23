
#include <thread>
#include "webserver.h"

int main(int argc, char ** argv) {
    svher::LogAppender::ptr file_appender(new svher::FileLogAppender("./log.txt"));
    svher::LogFormatter::ptr fmt(new svher::LogFormatter("%d%T%m%n"));
//    file_appender->setFormatter(fmt);
//    file_appender->setLevel(svher::LogLevel::ERROR);
//    logger->addAppender(file_appender);
    LOG_LEVEL(LOG_ROOT(), svher::LogLevel::DEBUG) << "Hello world" << "Yes";
    LOG_DEBUG_FORMAT(LOG_ROOT(), "Hello %d", 12);

    auto l = svher::LoggerMgr::GetInstance()->getLogger("xx");
    LOG_LEVEL(l, svher::LogLevel::DEBUG) << "Hello world";
}