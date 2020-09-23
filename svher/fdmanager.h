#pragma once

#include <memory>
#include "thread.h"
#include "iomanager.h"
#include "singleton.h"

namespace svher {
    class FdContext : public std::enable_shared_from_this<FdContext> {
    public:
        typedef std::shared_ptr<FdContext> ptr;
        explicit FdContext(int fd);
        ~FdContext();

        bool init();
        bool isInit() const { return m_isInit; }
        bool isSocket() const { return m_isSocket; }
        bool isClosed() const { return m_isClosed; }
        bool close();
        bool getUserNonblock() const { return m_userNonblock; }
        void setUserNonblock(bool v) { m_userNonblock = v; }
        bool getSysNonblock() const { return m_sysNonblock; }
        void setTimeout(int type, uint64_t v);
        uint64_t getTimeout(int type);
    private:
        bool m_isInit = false;
        bool m_isSocket = false;
        bool m_sysNonblock = false;
        bool m_userNonblock = false;
        bool m_isClosed = false;
        int m_fd;
        uint64_t m_recvTimeout = -1;
        uint64_t m_sendTimeout = -1;
        svher::IOManager* m_iomanager;
    };

    class FdManager {
    public:
        typedef RWMutex RWMutexType;
        FdManager();
        FdContext::ptr get(int fd, bool auto_create = false);
        void del(int fd);
    private:
        RWMutexType m_mutex;
        std::vector<FdContext::ptr> m_datas;
    };

    typedef Singleton<FdManager> FdMgr;
}