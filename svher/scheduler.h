#pragma once
#include "fiber.h"
#include "thread.h"
#include <utility>
#include <vector>
#include <list>

namespace svher {

    class Scheduler {
    public:
        typedef Mutex MutexType;
        typedef std::shared_ptr<Scheduler> ptr;
        explicit Scheduler(size_t threads = 1, bool use_caller = false, const std::string& name = "");
        virtual ~Scheduler();
        const std::string& getName() const { return m_name; }
        static Scheduler* GetThis();
        static Fiber* GetMainFiber();
        void start();
        void stop();
        template<class T>
        void schedule(T callback, int thread = -1) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(callback, thread);
            }
            if (need_tickle) tickle();
        }
        template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while(begin != end) {
                    need_tickle = scheduleNoLock(&*begin) || need_tickle;
                }
            }
            if (need_tickle) tickle();
        }

    protected:
        virtual void tickle();
        void run();
        virtual bool stopping();
        virtual bool idle();
        void setThis();
    private:
        template<class T>
        bool scheduleNoLock(T callback, int thread) {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(callback, thread);
            if (ft.fiber || ft.cb) {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }
        struct FiberAndThread {
            Fiber::ptr fiber;
            std::function<void()> cb;
            // 在这个线程上执行
            int thread;

            FiberAndThread(Fiber::ptr f, int thr)
                : fiber(std::move(f)), thread(thr) {}
            FiberAndThread(Fiber::ptr* f, int thr)
                : thread(thr) {
                // 让上层引用计数减 1
                fiber.swap(*f);
            }
            FiberAndThread(std::function<void()> f, int thr)
                : cb(std::move(f)), thread(thr) {}
            FiberAndThread(std::function<void()>* f, int thr)
                    : thread(thr) {
                // 让上层引用计数减 1
                cb.swap(*f);
            }
            FiberAndThread() : thread(-1) {}
            void reset() {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };
        MutexType m_mutex;
        std::vector<Thread::ptr> m_threads;
        std::list<FiberAndThread> m_fibers;
        Fiber::ptr m_rootFiber;
        std::string m_name;
    protected:
        std::vector<int> m_threadIds;
        size_t m_threadCount = 0;
        std::atomic_size_t m_activeThreadCount{0};
        std::atomic_size_t m_idleThreadCount{0};
        bool m_stopping = true;
        bool m_autoStop = false;
        int m_rootThread = 0;
    };
}