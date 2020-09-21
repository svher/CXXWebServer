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
        ASSERT(ret);
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];
        ret = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        ASSERT(ret);
        ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        ASSERT(ret);
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
        if (m_fdContexts.size() > fd) {
            fd_ctx = m_fdContexts[fd];
        } else {
            lock.unlock();
            RWMutexType::WriteLock lk(m_mutex);
            contextResize(m_fdContexts.size() * 1.5);
            fd_ctx = m_fdContexts[fd];
        }
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (fd_ctx->event & event) {
            LOG_ERROR(g_logger) << "add duplicate event fd="
                        << fd << " event=" << event
                        << " fd_ctx.event=" << fd_ctx->event;
            ASSERT(false);
        }
        int op = fd_ctx->event ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epollEvent;
        epollEvent.events = EPOLLET | fd_ctx->event | event;
        epollEvent.data.ptr = fd_ctx;

        int ret = epoll_ctl(m_epfd, op, fd, &epollEvent);
        if (ret) {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << fd << ", " << epollEvent.events
                    << "): " << ret << " (" << errno << ", " <<
                    strerror(errno) << ")";
        }
        ++m_pendingEventCount;
        fd_ctx->event = (Event)(fd_ctx->event | event);
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
        return false;
    }

    bool IOManager::cancelEvent(int fd, IOManager::Event event) {
        return false;
    }

    bool IOManager::cancelAll(int fd) {
        return false;
    }

    IOManager *IOManager::GetThis() {
        return nullptr;
    }

    bool IOManager::idle() {
        return Scheduler::idle();
    }

    bool IOManager::stopping() {
        return Scheduler::stopping();
    }

    void IOManager::tickle() {
        Scheduler::tickle();
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
}