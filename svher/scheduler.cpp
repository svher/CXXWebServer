#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace svher {
    static Logger::ptr g_logger = LOG_NAME("sys");

    static thread_local Scheduler* t_scheduler = nullptr;
    static thread_local Fiber* t_fiber = nullptr;

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) : m_name(name) {
        ASSERT(threads > 0);
        LOG_INFO(g_logger) << "Scheduler threads_num: " << threads << " use_caller: " << use_caller
                                << " name: " << name;
        if (use_caller) {
            Fiber::GetThis();
            --threads;
            // 只允许有一个协程调度器
            ASSERT(GetThis() == nullptr);
            t_scheduler = this;
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            if (!m_name.empty()) Thread::SetName(m_name);
            t_fiber = m_rootFiber.get();
            m_rootThread = svher::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        } else {
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler() {
        ASSERT(m_stopping);
        if (GetThis() == this) {
            t_scheduler = nullptr;
        }
    }

    Scheduler *Scheduler::GetThis() {
        return t_scheduler;
    }

    Fiber *Scheduler::GetMainFiber() {
        return t_fiber;
    }

    void Scheduler::start() {
        MutexType::Lock lock(m_mutex);
        if (!m_stopping) {
            return;
        }
        m_stopping = false;
        ASSERT(m_threads.empty());
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i) {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                               m_name + "_" + std::to_string(i)));
            // 放信号量保证构造完毕后 getId 总可成功返回
            m_threadIds.push_back(m_threads[i]->getId());
        }
        lock.unlock();
    }

    void Scheduler::stop() {
        m_autoStop = true;
        if (m_rootFiber && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
            || m_rootFiber->getState() == Fiber::INIT)) {
            LOG_INFO(g_logger) << this->m_name << " stopped";
            m_stopping = true;

            // use_caller 且只有线程
            if (stopping()) {
                return;
            }
        }
        // use_caller 要在创建 scheduler 线程中执行
        if (m_rootThread != -1) {
            ASSERT(GetThis() == this);
        } else {
            ASSERT(GetThis() != this);
        }
        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i) {
            tickle();
        }
        if (m_rootFiber) {
            if (!stopping()) m_rootFiber->call();
        }
        std::vector<Thread::ptr> threads;
        {
            MutexType::Lock lock(m_mutex);
            threads.swap(m_threads);
        }
        for (auto& thread : threads) {
            thread->join();
        }
        if (stopping()) {
            return;
        }
    }

    void Scheduler::tickle() {
        LOG_INFO(g_logger) << "tickle";
    }

    void Scheduler::run() {
        LOG_INFO(g_logger) << "run";
        setThis();
        if (GetThreadId() != m_rootThread) {
            t_fiber = Fiber::GetThis().get();
        }
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        LOG_INFO(g_logger) << "init idle fiber, id: " << idle_fiber->getId();
        Fiber::ptr cb_fiber;

        FiberAndThread ft;
        while (true) {
            ft.reset();
            bool tickle_me = false;
            bool is_activate = false;
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end()) {
                    if (it->thread != -1 && it->thread != GetThreadId()) {
                        ++it;
                        // 再发起信号让别的线程执行
                        tickle_me = true;
                        continue;
                    }
                    ASSERT(it->fiber || it->cb);
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                        ++it;
                        continue;
                    }
                    // 取出一个需要执行的任务
                    ft = *it;
                    m_fibers.erase(it);
                    ++m_activeThreadCount;
                    is_activate = true;
                    break;
                }
            }
            if (tickle_me) {
                tickle();
            }
            if (ft.fiber && ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->swapIn();
                --m_activeThreadCount;
                if (ft.fiber->getState() == Fiber::READY) {
                    schedule(ft.fiber);
                } else if (ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                    ft.fiber->m_state = Fiber::HOLD;
                }
                ft.reset();
            } else if (ft.cb) {
                if (cb_fiber) {
                    cb_fiber->reset(ft.cb);
                } else {
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();
                cb_fiber->swapIn();
                --m_activeThreadCount;
                if (cb_fiber->getState() == Fiber::READY) {
                    schedule(ft.fiber);
                    cb_fiber.reset();
                } else if (cb_fiber->getState() == Fiber::TERM
                           || cb_fiber->getState() == Fiber::EXCEPT) {
                    cb_fiber->reset(nullptr);
                } else {
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            } else {
                // 取出的工作项无效，活跃线程数减一
                if (is_activate) {
                    --m_activeThreadCount;
                    continue;
                }
                if (idle_fiber->getState() == Fiber::TERM) {
                    LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }
                ++m_idleThreadCount;
                idle_fiber->swapIn();
                if (idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                    idle_fiber->m_state = Fiber::HOLD;
                }
                --m_idleThreadCount;
            }
        }
    }

    bool Scheduler::stopping() {
        MutexType::Lock lock(m_mutex);
        return m_autoStop && m_stopping
            && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::setThis() {
        t_scheduler = this;
    }

    bool Scheduler::idle() {
        while (!stopping()) {
            Fiber::YieldToHold();
        }
        return false;
    }
}