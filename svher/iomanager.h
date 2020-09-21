#pragma once
#include "scheduler.h"

namespace svher {
    class IOManager : public Scheduler {
        struct FdContext;
    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;
        enum Event {
            NONE = 0,
            READ = 1,
            WRITE = 2
        };
        IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
        ~IOManager();
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool delEvent(int fd, Event event);
        bool cancelEvent(int fd, Event event);
        bool cancelAll(int fd);
        static IOManager* GetThis();
    protected:
        bool idle() override;
        bool stopping() override;
        void tickle() override;
        void contextResize(size_t size);
        int m_tickleFds[2];
        std::atomic_size_t m_pendingEventCount{0};
        RWMutexType m_mutex;
        std::vector<FdContext*> m_fdContexts;
    private:
        int m_epfd = 0;
        struct FdContext {
            typedef Mutex MutexType;
            struct EventContext {
                Scheduler* scheduler; // 事件执行的 scheduler
                std::shared_ptr<Fiber> fiber;
                std::function<void()> cb;
            };
            EventContext& getContext(Event event);
            int fd = 0;
            EventContext read;
            EventContext write;
            Event event = NONE;
            MutexType mutex;
        };
    };
}