#pragma once
#include "scheduler.h"
#include "timer.h"

namespace svher {
    class IOManager : public Scheduler, public TimerManager {
        struct FdContext;
    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;
        enum Event {
            NONE = 0,
            READ = 1,  // EPOLLIN
            WRITE = 4  // EPOLLOUT
        };
        IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
        ~IOManager();
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool delEvent(int fd, Event event);
        bool cancelEvent(int fd, Event event);
        bool cancelAll(int fd);
        static IOManager* GetThis();
    protected:
        void idle() override;
        bool stopping() override;
        bool stopping(uint64_t timeout);
        void tickle() override;
        void contextResize(size_t size);
        int m_tickleFds[2]{};
        void onTimerInsertedAtFront() override;
    private:
        int m_epfd = 0;
        std::atomic_size_t m_pendingEventCount{0};
        RWMutexType m_mutex;
        std::vector<FdContext*> m_fdContexts;
        struct FdContext {
            typedef Mutex MutexType;
            struct EventContext {
                Scheduler* scheduler; // 事件执行的 scheduler
                std::shared_ptr<Fiber> fiber;
                std::function<void()> cb;
            };
            void triggerEvent(Event event);
            EventContext& getContext(Event event);
            void resetContext(EventContext& ctx);
            int fd = 0;
            EventContext read;
            EventContext write;
            Event events = NONE;
            MutexType mutex;
        };
    };
}