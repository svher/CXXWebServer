#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <list>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <functional>
#include <fstream>
#include <ctime>
#include <cstdarg>
#include "singleton.h"
#include "util.h"

#define LOG_LEVEL(logger, level) \
    if (logger->getLevel() <= level)   \
        svher::LogEventWrap(svher::LogEvent::ptr(new svher::LogEvent(logger, level, __FILE__, __LINE__, 0, \
            svher::GetThreadId(), svher::GetFiberId(), time(0)))).getSS()

#define LOG_DEBUG(logger) LOG_LEVEL(logger, svher::LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, svher::LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, svher::LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, svher::LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, svher::LogLevel::FATAL)

#define LOG_LEVEL_FORMAT(logger, level, fmt, ...) \
    if (logger->getLevel() <= level)   \
        svher::LogEventWrap(svher::LogEvent::ptr(new svher::LogEvent(logger, level, __FILE__, __LINE__, 0, \
            svher::GetThreadId(), svher::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define LOG_DEBUG_FORMAT(logger, fmt, ...) LOG_LEVEL_FORMAT(logger, svher::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_INFO_FORMAT(logger, fmt, ...) LOG_LEVEL_FORMAT(logger, svher::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARN_FORMAT(logger, fmt, ...) LOG_LEVEL_FORMAT(logger, svher::LogLevel::WARN, __VA_ARGS__)
#define LOG_ERROR_FORMAT(logger, fmt, ...) LOG_LEVEL_FORMAT(logger, svher::LogLevel::ERROR, __VA_ARGS__)
#define LOG_FATAL_FORMAT(logger, fmt, ...) LOG_LEVEL_FORMAT(logger, svher::LogLevel::FATAL, __VA_ARGS__)

#define LOG_ROOT() svher::LoggerMgr::GetInstance()->getRoot()

namespace svher {

    class Logger;

    class LogLevel {
    public:
        enum Level {
            UNKNOWN = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };

        static const char* ToString(Level level);
    };

    // 日志事件
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time);
        ~LogEvent();
        const char* getFile() const {
            return m_file;
        }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_ss.str(); }
        std::stringstream& getSS() { return m_ss; }
        std::shared_ptr<Logger> getLogger() { return m_logger; }
        LogLevel::Level getLevel() {  return m_level; }
        void format(const char* fmt, ...);
    private:
        const char* m_file = nullptr; // 文件名
        int32_t m_line = 0;             // 行号
        uint32_t m_elapse = 0;          // 程序启动到现在的毫秒数
        uint32_t m_threadId = 0;        // 线程 Id
        uint32_t m_fiberId = 0;         // 协程 Id
        uint64_t m_time;                // 时间戳
        std::stringstream m_ss;
        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
    };

    class LogEventWrap {
    public:
        explicit LogEventWrap(LogEvent::ptr e);
        ~LogEventWrap();
        std::stringstream &getSS();
        LogEvent::ptr getEvent() { return m_event; }
    private:
        LogEvent::ptr m_event;
    };

    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(const std::string& pattern);
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
        class FormatItem {
        public:
            typedef std::shared_ptr<LogFormatter::FormatItem> ptr;
            FormatItem(const std::string& fmt) {}
            virtual ~FormatItem() {}
            virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };
    private:
        void init();
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
    };

    // 日志输出地
    class LogAppender {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender() {}

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
        void setLevel(LogLevel::Level level) { m_level = level; }
        LogLevel::Level getLevel() const { return m_level; }
        LogFormatter::ptr getFormatter() const { return m_formatter; }
    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;
        LogFormatter::ptr m_formatter;
    };

    // 日志输出器
    class Logger : public std::enable_shared_from_this<Logger> {
    public:
        typedef std::shared_ptr<Logger> ptr;
        Logger(const std::string& name = "root") : m_name(name) {
            // %m -- 消息体
            // %p -- level
            // %r -- 启动后的时间
            // %c -- 日志名称
            // %t -- 线程 ID
            // %n -- 回车
            // %d -- 时间
            // %f -- 文件名
            // %l -- 行号
            // %T -- Tab
            // %F -- Fiber
            m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T<%f:%l>%T%m%n"));
        }
        void log(LogLevel::Level level, const LogEvent::ptr event);
        void info(const LogEvent::ptr event);
        void debug(const LogEvent::ptr event);
        void warn(const LogEvent::ptr event);
        void error(const LogEvent::ptr event);
        void fatal(const LogEvent::ptr event);
        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level level) { m_level = level; }
        const std::string getName() const { return m_name; }
    private:
        std::string m_name;
        LogLevel::Level m_level = LogLevel::DEBUG;
        std::list<LogAppender::ptr> m_appenders;
        LogFormatter::ptr m_formatter;
    };

    class StdOutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdOutLogAppender> ptr;
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    private:
    };

    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdOutLogAppender> ptr;
        FileLogAppender(const std::string& filename);
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
        bool reopen();
    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };

    class LoggerManager {
    public:
        LoggerManager();
        Logger::ptr getLogger(const std::string& name);
        void init();
        Logger::ptr getRoot() const { return m_root; }
    private:
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    typedef SingletonPtr<LoggerManager> LoggerMgr;
}