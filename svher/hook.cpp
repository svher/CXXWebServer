#include <dlfcn.h>
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "hook.h"
#include "fdmanager.h"
#include "iomanager.h"
#include "fiber.h"
#include "log.h"
#include "config.h"

namespace svher {

    static svher::Logger::ptr g_logger = LOG_NAME("sys");

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

    static svher::ConfigVar<int>::ptr g_tcp_connect_timeout =
            Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
    static thread_local bool t_hook_enable = false;
    bool is_hook_enable() {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) {
        t_hook_enable = flag;
    }

    void hook_init() {
        static bool is_inited = false;
        if (is_inited) return;
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
        /*
         * sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
         * usleep_f = (usleep_fun)dlsym(RTLD_NEXT, "usleep");
         */
        HOOK_FUN(XX)
#undef XX
    }

    static uint64_t s_connect_timeout = -1;

    struct HookIniter {
        HookIniter() {
            hook_init();
            s_connect_timeout = g_tcp_connect_timeout->getValue();
            g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_value
                                    << " to " << new_value;
                s_connect_timeout = new_value;
            });
        }
    };

    struct timer_info {
        int cancelled = 0;
    };
    // 全局变量在 Hook 前被初始化
    static HookIniter s_hook_initer;

    template<typename OriginFunc, typename ... Args>
    static size_t do_io(int fd, OriginFunc func, const char* hook_func_name,
                        uint32_t event, int timeout_so,
                        Args&& ... args) {
        if (!svher::t_hook_enable) {
            return func(fd, std::forward<Args>(args)...);
        }

        FdContext::ptr ctx = svher::FdMgr::GetInstance()->get(fd);
        if (!ctx) {
            return func(fd, std::forward<Args>(args)...);
        }
        if (ctx->isClosed()) {
            errno = EBADF;
            return -1;
        }
        // 由于我们是把同步转异步，所以如果用户主动设置了非阻塞
        // 和我们默认设置的 Nonblock 行为一致，不需要处理
        if (!ctx->isSocket() || ctx->getUserNonblock()) {
            return func(fd, std::forward<Args>(args)...);
        }
        uint64_t to = ctx->getTimeout(timeout_so);
        std::shared_ptr<timer_info> tinfo(new timer_info);
        retry:
        // TODO ssize_t?
        ssize_t n = func(fd, std::forward<Args>(args)...);
        while(n == -1 && errno == EINTR) {
            n = func(fd, std::forward<Args>(args)...);
        }
        if (n == -1 && errno == EAGAIN) {
            LOG_DEBUG(g_logger) << "do_io<" << hook_func_name << ">";
            IOManager* ioManager = IOManager::GetThis();
            Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);
            // 有设置超时
            if (to != (uint64_t)-1) {
                timer = ioManager->addConditionalTimer(to, [winfo, fd, ioManager, event]() {
                    auto t = winfo.lock();
                    if (!t || t->cancelled) return;
                    t->cancelled = ETIMEDOUT;
                    ioManager->cancelEvent(fd, (IOManager::Event)event);
                }, winfo);
            }

            int ret = ioManager->addEvent(fd, (IOManager::Event)event);
            if (ret == -1) {
                LOG_ERROR(g_logger) << hook_func_name << " addEvent("
                                    << fd << ", " << event << ")";
                if (timer) {
                    timer->cancel();
                }
                return -1;
            } else {
                Fiber::YieldToHold();
                if (timer) {
                    timer->cancel();
                }
                if (tinfo->cancelled) {
                    errno = tinfo->cancelled;
                    return -1;
                }
                goto retry;
            }
        }
        return n;
    }
}

extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX)
#undef XX

    unsigned int sleep(unsigned int seconds) {
        if (!svher::t_hook_enable) return sleep_f(seconds);
        svher::Fiber::ptr fiber = svher::Fiber::GetThis();
        svher::IOManager* iomanager = svher::IOManager::GetThis();
        iomanager->addTimer(seconds * 1000, [iomanager, fiber]() {
            iomanager->schedule(fiber);
        });
        svher::Fiber::YieldToHold();
        return 0;
    }

    int usleep(useconds_t usec){
        if (!svher::t_hook_enable) return usleep_f(usec);
        svher::Fiber::ptr fiber = svher::Fiber::GetThis();
        svher::IOManager* iomanager = svher::IOManager::GetThis();
        iomanager->addTimer(usec / 1000, [iomanager, fiber]() {
            iomanager->schedule(fiber);
        });
        svher::Fiber::YieldToHold();
        return 0;
    }

    int nanosleep(const struct timespec *req, struct timespec *rem) {
        if (!svher::t_hook_enable) return nanosleep_f(req, rem);
        uint64_t timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        svher::Fiber::ptr fiber = svher::Fiber::GetThis();
        svher::IOManager* iomanager = svher::IOManager::GetThis();
        iomanager->addTimer(timeout_ms, [iomanager, fiber]() {
            iomanager->schedule(fiber);
        });
        svher::Fiber::YieldToHold();
        return 0;
    }

    int socket(int domain, int type, int protocol) {
        if (!svher::t_hook_enable) {
            return socket_f(domain, type, protocol);
        }
        int fd = socket_f(domain, type, protocol);
        if (fd == -1) return fd;
        svher::FdMgr::GetInstance()->get(fd, true);
        return fd;
    }

    int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms) {
        if (!svher::t_hook_enable) {
            return connect_f(sockfd, addr, addrlen);
        }

        svher::FdContext::ptr ctx = svher::FdMgr::GetInstance()->get(sockfd);
        if (!ctx || ctx->isClosed()) {
            errno = EBADF;
            return -1;
        }
        if (!ctx->isSocket() || ctx->getUserNonblock()) {
            return connect_f(sockfd, addr, addrlen);
        }
        int n = connect_f(sockfd, addr, addrlen);
        if (n == 0) return 0;
        else if (n != -1 || errno != EINPROGRESS) {
            return n;
        }
        svher::IOManager* ioManager = svher::IOManager::GetThis();
        svher::Timer::ptr timer;
        std::shared_ptr<svher::timer_info> tinfo(new svher::timer_info);
        std::weak_ptr<svher::timer_info> winfo(tinfo);
        if (timeout_ms != (uint64_t)-1) {
            timer = ioManager->addConditionalTimer(timeout_ms, [winfo, sockfd, ioManager]() {
                auto t = winfo.lock();
                if (!t || t->cancelled) return;
                t->cancelled = ETIMEDOUT;
                ioManager->cancelEvent(sockfd, svher::IOManager::WRITE);
            }, winfo);
        }
        int ret = ioManager->addEvent(sockfd, svher::IOManager::WRITE);
        if (ret == 0) {
            svher::Fiber::YieldToHold();
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
        } else {
            if (timer) {
                timer->cancel();
            }
            LOG_ERROR(svher::g_logger) << "connect addEvent(" << sockfd <<", WRITE) error";
        }
        int error = 0;
        socklen_t len = sizeof(len);
        if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
            return -1;
        }
        if (!error) return 0;
        else {
            errno = error;
            return -1;
        }
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
        return connect_with_timeout(sockfd, addr, addrlen, svher::s_connect_timeout);
    }

    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
        int fd = do_io(sockfd, accept_f, "accept",
                       svher::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0) {
            svher::FdMgr::GetInstance()->get(fd, true);
        }
        return fd;
    }

    ssize_t read(int fd, void *buf, size_t count) {
        return svher::do_io(fd, read_f, "read",
                            svher::IOManager::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
        return svher::do_io(fd, readv_f, "readv", svher::IOManager::READ,
                            SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
        return svher::do_io(sockfd, recv_f, "recv", svher::IOManager::READ,
                            SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen) {
        return svher::do_io(sockfd, recvfrom_f, "recvfrom", svher::IOManager::READ,
                            SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
        return svher::do_io(sockfd, recvmsg_f, "recvmsg", svher::IOManager::READ,
                            SO_RCVTIMEO, msg, flags);
    }


    ssize_t write(int fd, const void *buf, size_t count) {
        return svher::do_io(fd, write_f, "write", svher::IOManager::WRITE,
                            SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
        return svher::do_io(fd, writev_f, "writev", svher::IOManager::WRITE,
                            SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
        return svher::do_io(sockfd, send_f, "send", svher::IOManager::WRITE,
                            SO_SNDTIMEO, buf, len, flags);
    }

    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
        return svher::do_io(sockfd, sendto_f, "sendto", svher::IOManager::WRITE,
                            SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
    }

    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
        return svher::do_io(sockfd, sendmsg_f, "sendmsg", svher::IOManager::WRITE,
                            SO_SNDTIMEO, msg, flags);
    }

    int close(int fd) {
        if (!svher::t_hook_enable) return close_f(fd);
        svher::FdContext::ptr ctx = svher::FdMgr::GetInstance()->get(fd);
        if (ctx) {
            auto iomanager = svher::IOManager::GetThis();
            if (iomanager) {
                iomanager->cancelAll(fd);
            }
            svher::FdMgr::GetInstance()->del(fd);
        }
        return close_f(fd);
    }

    int fcntl(int fd, int cmd, ... /* arg */ ) {
        va_list va;
        va_start(va, cmd);
        switch (cmd) {
            // int
            case F_SETFL: {
                int arg = va_arg(va, int);
                va_end(va);
                svher::FdContext::ptr ctx = svher::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                // 忽略用户主动设置的 Nonblock 标志位
                if (ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            case F_GETFL: {
                va_end(va);
                int ret = fcntl_f(fd, cmd);
                svher::FdContext::ptr ctx = svher::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClosed() || ctx->isSocket()) {
                    return ret;
                }
                if (ctx->getUserNonblock()) {
                    return ret | O_NONBLOCK;
                } else {
                    return ret & ~O_NONBLOCK;
                }
            }
            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
            case F_SETPIPE_SZ:
            case F_ADD_SEALS: {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            // void
            case F_GETFD:
            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
            case F_GETPIPE_SZ:
            case F_GET_SEALS:
                return fcntl_f(fd, cmd);
            // struct flock *
            case F_SETLK:
            case F_SETLKW:
            case F_GETLK: {
                struct flock *arg = va_arg(va, struct flock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            // struct f_owner_ex *
            case F_GETOWN_EX:
            case F_SETOWN_EX: {
                struct f_owner_ex *arg = va_arg(va, struct f_owner_ex *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            default:
                va_end(va);
                return fcntl_f(fd, cmd);
        }
    }

    int ioctl(int fd, unsigned long int request, ...) {
        va_list va;
        va_start(va, request);
        void* arg = va_arg(va, void*);
        va_end(va);
        if (request == FIONBIO) {
            // true: 允许非阻塞
            bool user_nonblock = !!*(int*)arg;
            svher::FdContext::ptr ctx = svher::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClosed() || ctx->isSocket()) {
                return ioctl_f(fd, request, arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(fd, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
        if (!svher::t_hook_enable) return setsockopt_f(sockfd, level, optname, optval, optlen);
        if (level == SOL_SOCKET) {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
                svher::FdContext::ptr ctx = svher::FdMgr::GetInstance()->get(sockfd);
                if (ctx) {
                    const auto* tv = (const timeval*)optval;
                    ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}