#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include <memory.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

namespace svher {

    Logger::ptr g_logger = LOG_NAME("sys");

    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name) {
        // Since Linux 2.6.8, the size argument is ignored
        m_epfd = epoll_create(5000);
        ASSERT(m_epfd > 0);
        int ret = pipe(m_tickleFds);
        ASSERT(!ret);
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];
        ret = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        ASSERT(!ret);
        ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        ASSERT(!ret);
        contextResize(64);
        start();
    }

    IOManager::~IOManager() {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);
        for (size_t i = 0; i < m_fdContexts.size(); ++i) {
            if (m_fdContexts[i]) {
                delete m_fdContexts[i];
            }
        }
    }

    int IOManager::addEvent(int fd, IOManager::Event event, std::function<void()> cb) {
        FdContext* fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd) {
            fd_ctx = m_fdContexts[fd];
        } else {
            lock.unlock();
            RWMutexType::WriteLock lk(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (fd_ctx->events & event) {
            LOG_ERROR(g_logger) << "add duplicate event fd="
                        << fd << " event=" << event
                        << " fd_ctx.events=" << fd_ctx->events;
            ASSERT(false);
        }
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epollEvent;
        epollEvent.events = EPOLLET | fd_ctx->events | event;
        epollEvent.data.ptr = fd_ctx;

        int ret = epoll_ctl(m_epfd, op, fd, &epollEvent);
        if (ret) {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << fd << ", " << epollEvent.events
                    << "): " << ret << " (" << errno << ", " <<
                    strerror(errno) << ")";
        }
        ++m_pendingEventCount;
        fd_ctx->events = (Event)(fd_ctx->events | event);
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        ASSERT(!event_ctx.scheduler && !event_ctx.fiber);
        event_ctx.scheduler = Scheduler::GetThis();
        if (cb) {
            event_ctx.cb.swap(cb);
        } else {
            event_ctx.fiber = Fiber::GetThis();
            ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, IOManager::Event event) {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() < fd) {
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();
        FdContext::MutexType::Lock lock1(fd_ctx->mutex);
        if (!(fd_ctx->events & event)) {
            return false;
        }
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;
        int ret = epoll_ctl(m_epfd, op, fd, &epevent);
        if (ret) {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << op << ", " << fd << ", " << epevent.events
                                << "): " << ret << " (" << errno << ", " <<
                                strerror(errno) << ")";
            return false;
        }
        --m_pendingEventCount;
        fd_ctx->events = new_events;
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    bool IOManager::cancelEvent(int fd, IOManager::Event event) {
        LOG_INFO(g_logger) << "Cancel event, fd: " << fd;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd) {
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();
        FdContext::MutexType::Lock lock1(fd_ctx->mutex);
        if (!(fd_ctx->events & event)) {
            return false;
        }
        LOG_INFO(g_logger) << "Cancel event, fd: " << fd;
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        LOG_INFO(g_logger) << "cancel op: " << op;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;
        int ret = epoll_ctl(m_epfd, op, fd, &epevent);
        if (ret) {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << op << ", " << fd << ", " << epevent.events
                                << "): " << ret << " (" << errno << ", " <<
                                strerror(errno) << ")";
            return false;
        }

        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        fd_ctx->events = new_events;
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    bool IOManager::cancelAll(int fd) {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() < fd) {
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();
        FdContext::MutexType::Lock lock1(fd_ctx->mutex);
        if (!fd_ctx->events) {
            return false;
        }
        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;
        int ret = epoll_ctl(m_epfd, op, fd, &epevent);
        if (ret) {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                << op << ", " << fd << ", " << epevent.events
                                << "): " << ret << " (" << errno << ", " <<
                                strerror(errno) << ")";
            return false;
        }

        if (fd_ctx->events & READ) {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        } else {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }
        ASSERT(fd_ctx->events == 0);
        return true;
    }

    IOManager *IOManager::GetThis() {
        return dynamic_cast<IOManager*>(Scheduler::GetThis());
    }

    void IOManager::idle() {
        epoll_event* events = new epoll_event[64]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {
            delete[] ptr;
        });
        while (true) {
            uint64_t next_timeout = getNextTimer();
            if (stopping(next_timeout)) {
                LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
                break;
            }

            int ret = 0;
            do {
                static const int MAX_TIMEOUT = 500;
                if (next_timeout != -1ull) {
                    next_timeout = next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
                } else {
                    next_timeout = MAX_TIMEOUT;
                }
                ret = epoll_wait(m_epfd, events, 64, (int)next_timeout);
                if (!(ret < 0 && errno == EINTR)) {
                    break;
                }
            } while(true);

            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if (!cbs.empty()) {
                LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            for (int i = 0; i < ret; ++i) {
                epoll_event& event = events[i];
                if (event.data.fd == m_tickleFds[0]) {
                    uint8_t dummy;
                    // ET 触发，不读干净就不会再通知
                    while (read(m_tickleFds[0], &dummy, 1) == 1);
                    continue;
                }
                FdContext* fd_ctx = (FdContext*)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP)) {
                    event.events |= EPOLLIN | EPOLLOUT;
                }
                int real_events = NONE;
                if (event.events & EPOLLIN) {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT) {
                    real_events |= WRITE;
                }
                if ((fd_ctx->events & real_events) == NONE) {
                    continue;
                }
                int left_events = (fd_ctx->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;
                int ret2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (ret2) {
                    LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                        << op << ", " << fd_ctx->fd << ", " << event.events
                                        << "): " << ret << " (" << errno << ", " <<
                                        strerror(errno) << ")";
                    continue;
                }
                if (real_events & READ) {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE) {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }
            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();
            raw_ptr->swapOut();
        }
    }

    bool IOManager::stopping() {
        return false;
    }

    void IOManager::tickle() {
        if (hasIdleThreads()) return;
        int ret = write(m_tickleFds[1], "T", 1);
        ASSERT(ret == 1);

    }

    void IOManager::contextResize(size_t size) {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < m_fdContexts.size(); ++i) {
            if (!m_fdContexts[i]) {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }

    }

    void IOManager::onTimerInsertedAtFront() {
        tickle();
    }

    bool IOManager::stopping(uint64_t timeout) {
        timeout = getNextTimer();
        return timeout == -1ull && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    void IOManager::FdContext::triggerEvent(IOManager::Event event) {
        ASSERT(events & event);
        events = (Event)(events & ~event);
        EventContext& ctx = getContext(event);
        if (ctx.cb) {
            // 传指针让 ctx.cb 失效
            ctx.scheduler->schedule(&ctx.cb);
        } else {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler = nullptr;
        return;
    }

    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(IOManager::Event event) {
        switch(event) {
            case IOManager::READ:
                return read;
            case IOManager::WRITE:
                return write;
            default:
                ASSERT2(false, "getContext");
        }
    }

    void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext &ctx) {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }
}