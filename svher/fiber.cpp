#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "scheduler.h"

namespace svher {
    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};

    // 当前线程正在执行的协程
    static thread_local Fiber* t_fiber = nullptr;
    // 线程的主协程
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    static Logger::ptr g_logger = LOG_NAME("sys.fiber");

    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
            Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

    class MallocStackAllocator {
    public:
        static void* Alloc(size_t size) {
            return malloc(size);
        }
        static void Dealloc(void* vp, size_t sz) {
            return free(vp);
        }
    };

    using StackAlloc = MallocStackAllocator;

    Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
        : m_id(++s_fiber_id), m_useCaller(use_caller), m_cb(std::move(cb)) {
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
        m_stack = StackAlloc::Alloc(m_stacksize);
        if (getcontext(&m_ctx)) {
            ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        // 如果线程中直接执行协程而非调度器执行，则使用 CallerMainFunc
        if (use_caller)
            makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
        else {
            makecontext(&m_ctx, &Fiber::MainFunc, 0);
        }
        LOG_DEBUG(g_logger) << "Fiber::Fiber id: " << m_id;
    }

    Fiber::Fiber() {
        m_state = EXEC;
        SetThis(this);
        if (getcontext(&m_ctx)) {
            ASSERT2(false, "getcontext");
        }
        ++s_fiber_count;
        LOG_DEBUG(g_logger) << "Fiber::Fiber";
    }

    Fiber::~Fiber() {
        --s_fiber_count;
        if (m_stack) {
            ASSERT2(m_state == TERM || m_state == INIT || m_state == EXCEPT,
                   "id: " + std::to_string(m_id) + "state: " + std::to_string(m_state));
            StackAlloc::Dealloc(m_stack, m_stacksize);
        } else {
            // Is Main Fiber
            ASSERT(!m_cb && m_state == EXEC);
            Fiber* cur = t_fiber;
            if (cur == this) {
                SetThis(nullptr);
            }
        }
        LOG_DEBUG(g_logger) << "Fiber::~Fiber id: " << m_id;
    }

    void Fiber::reset(std::function<void()> cb) {
        ASSERT(m_stack);
        ASSERT(m_state == TERM || m_state == INIT);
        m_cb = std::move(cb);
        if (getcontext(&m_ctx)) {
            ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;
    }

    void Fiber::swapIn() {
        SetThis(this);
        ASSERT(m_state != EXEC);
        m_state = EXEC;
        if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
            ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::swapOut() {
        ASSERT(Scheduler::GetMainFiber()->m_state == EXEC);
        SetThis(Scheduler::GetMainFiber());
        if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
            ASSERT2(false, "swapcontext");
        }

    }

    void Fiber::callOut() {
        ASSERT(t_threadFiber->m_state == EXEC);
        SetThis(t_threadFiber.get());
        if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
            ASSERT2(false, "swapcontext");
        }
    }

    Fiber::ptr Fiber::GetThis() {
        if (t_fiber) {
            return t_fiber->shared_from_this();
        }
        // 没有主协程，则创建主协程，在 Fiber 构造函数里指定 t_fiber 的指针
        Fiber::ptr main_fiber(new Fiber);
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    void Fiber::YieldToReady() {
        Fiber::ptr cur = GetThis();
        cur->m_state = READY;
        if (cur->m_useCaller)
            cur->callOut();
        else
            cur->swapOut();
    }

    void Fiber::YieldToHold() {
        Fiber::ptr cur = GetThis();
        cur->m_state = HOLD;
        if (cur->m_useCaller)
            cur->callOut();
        else
            cur->swapOut();
    }

    uint64_t Fiber::TotalFibers() {
        return s_fiber_count;
    }

    void Fiber::MainFunc() {
        Fiber::ptr cur = GetThis();
        ASSERT(cur);
        try {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        } catch (std::exception& ex) {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber exception: " << ex.what()
                    << " fiber_id=" << cur->getId()
                    << std::endl << BacktraceToString();
        } catch (...) {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber exception: ";
        }
        // 销毁智能指针
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();

        // caller 线程调用 MainFunc 协程，且 GetMainFiber 存储的也是该协程
        // swapOut 相当于无效切换
        ASSERT2(false, "execution should never reach here");
    }

    void Fiber::SetThis(Fiber *f) {
        t_fiber = f;
    }

    uint64_t Fiber::GetFiberId() {
        if (t_fiber) {
            return t_fiber->getId();
        }
        return 0;
    }

    void Fiber::call() {
        SetThis(this);
        m_state = EXEC;
        ASSERT(GetThis() != t_threadFiber);
        if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
            ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::CallerMainFunc() {
        Fiber::ptr cur = GetThis();
        ASSERT(cur);
        try {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        } catch (std::exception& ex) {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber exception: " << ex.what()
                                << " fiber_id=" << cur->getId()
                                << std::endl << BacktraceToString();
        } catch (...) {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber exception: ";
        }
        // 销毁智能指针
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->callOut();

        // caller 线程调用 MainFunc 协程，且 GetMainFiber 存储的也是该协程
        // swapOut 相当于无效切换
        ASSERT2(false, "execution should never reach here");
    }
}

