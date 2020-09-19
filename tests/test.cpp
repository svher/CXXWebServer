#include <thread>
#include "log.h"
#include "util.h"

int main(int argc, char ** argv) {
    svher::Logger::ptr logger(new svher::Logger);
    logger->addAppender(svher::LogAppender::ptr(new svher::StdOutLogAppender));

    svher::LogAppender::ptr file_appender(new svher::FileLogAppender("./log.txt"));
    svher::LogFormatter::ptr fmt(new svher::LogFormatter("%d%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(svher::LogLevel::ERROR);
    logger->addAppender(file_appender);
    LOG_LEVEL(logger, svher::LogLevel::DEBUG) << "Hello world" << "Yes";
    LOG_DEBUG_FORMAT(logger, "Hello %d", 12);

    auto l = svher::LoggerMgr::GetInstance()->getLogger("xx");
    LOG_LEVEL(l, svher::LogLevel::DEBUG) << "Hello world";
}