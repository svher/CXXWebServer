#pragma once

#include "thread.h"
#include <set>
#include <vector>

namespace svher {
    class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer> {
        friend class TimerManager;
    public:
        typedef std::shared_ptr<Timer> ptr;
        bool cancel();
        bool reset(uint64_t ms, bool from_now);
        bool refresh();
    private:
        Timer(uint64_t ms, std::function<void()> cb,
              bool recurring, TimerManager* manager);
        Timer(uint64_t next);
    private:
        bool m_recurring = false;   // Is cycle
        uint64_t m_ms = 0;          // Period
        uint64_t m_next = 0;
        std::function<void()> m_cb;
        TimerManager* m_manager = nullptr;
        struct Comparator {
            bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
        };
    };

    class TimerManager {
        friend class Timer;
    public:
        typedef RWMutex RWMutexType;
        TimerManager();
        virtual ~TimerManager();
        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                            bool recurring = false);
        Timer::ptr addConditionalTimer(uint64_t ms, const std::function<void()>& cb,
                                       std::weak_ptr<void> weak_cond, bool recurring = false);
        uint64_t getNextTimer();
        void listExpiredCb(std::vector<std::function<void()>>& cbs);
        bool hasTimer();
    protected:
        virtual void onTimerInsertedAtFront() = 0;
        void addTimer(const Timer::ptr& timer, RWMutexType::WriteLock& lock);
    private:
        bool detectClockRollover(uint64_t now_ms);
        RWMutexType m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers;
        bool m_tickled = false;
        uint64_t m_previousTime = 0;
    };
}