#pragma once

#include <ucontext.h>
#include <memory>
#include "thread.h"

namespace svher {
    class Fiber : public std::enable_shared_from_this<Fiber> {
    public:
        friend class Scheduler;
        typedef std::shared_ptr<Fiber> ptr;
        enum State {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT
        };
        Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
        ~Fiber();
        // INIT TERM 可调用此函数
        void reset(std::function<void()> cb);

        void call();
        void callOut();
        uint64_t getId() const { return m_id; }
        State getState() const { return m_state; }
        static Fiber::ptr GetThis();
        static void YieldToReady();
        static void YieldToHold();
        static uint64_t TotalFibers();
        static void MainFunc();
        static void CallerMainFunc();
        static void SetThis(Fiber* f);
        static uint64_t GetFiberId();
    private:
        Fiber();
        void swapIn();
        void swapOut();
        uint64_t m_id = 0;
        uint32_t m_stacksize = 0;
        State m_state = INIT;
        ucontext_t m_ctx;
        void* m_stack = nullptr;
        bool m_useCaller;
        std::function<void()> m_cb;
    };
}