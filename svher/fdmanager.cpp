#include "fdmanager.h"
#include "hook.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>

namespace svher {
    FdContext::FdContext(int fd) : m_fd(fd) {
        init();
    }

    bool FdContext::init() {
        if (m_isInit) {
            return false;
        }
        m_recvTimeout = -1;
        m_sendTimeout = -1;
        struct stat fd_stat;
        if (fstat(m_fd, &fd_stat) == -1) {
            m_isInit = false;
            m_isSocket = false;
        } else {
            m_isInit = true;
            m_isSocket = S_ISSOCK(fd_stat.st_mode);
        }
        if (m_isSocket) {
            int flags = fcntl_f(m_fd, F_GETFL, 0);
            if (!(flags & O_NONBLOCK)) {
                m_sysNonblock = true;
            }
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        } else m_sysNonblock = false;
        m_userNonblock = false;
        m_isClosed = false;
        return m_isInit;
    }

    FdContext::~FdContext() {

    }

    bool FdContext::close() {
        return false;
    }

    void FdContext::setTimeout(int type, uint64_t v) {
        if (type == SO_RCVTIMEO)
            m_recvTimeout = v;
        else
            m_sendTimeout = v;
    }

    uint64_t FdContext::getTimeout(int type) {
        if (type == SO_RCVTIMEO)
            return m_recvTimeout;
        else
            return m_sendTimeout;
    }

    FdManager::FdManager() {
        m_datas.resize(64);
    }

    FdContext::ptr FdManager::get(int fd, bool auto_create) {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_datas.size() <= fd) {
            if (!auto_create) {
                return nullptr;
            }

        } else if (m_datas[fd] && !auto_create){
            return m_datas[fd];
        }
        lock.unlock();
        RWMutexType::WriteLock lock1(m_mutex);
        FdContext::ptr context(new FdContext(fd));
        m_datas[fd] = context;
        return context;
    }

    void FdManager::del(int fd) {
        RWMutexType::WriteLock  lock(m_mutex);
        if ((int)m_datas.size() <= fd) {
            return;
        }
        m_datas[fd].reset();
    }
}