#pragma once

#include <memory>
#include "address.h"
#include "util.h"

namespace svher {
    class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
    public:
        typedef std::shared_ptr<Socket> ptr;
        typedef std::weak_ptr<Socket> weak_ptr;
        Socket(int family, int type, int protocol = 0);

        enum Type {
            TCP = SOCK_STREAM,
            UDP = SOCK_DGRAM
        };

        enum Family {
            IPv4 = AF_INET,
            IPv6 = AF_INET6,
            Unix = AF_UNIX
        };

        static Socket::ptr CreateTCP(Address::ptr address);
        static Socket::ptr CreateUDP(Address::ptr address);

        static Socket::ptr CreateTCPSocket();
        static Socket::ptr CreateUDPSocket();

        static Socket::ptr CreateTCPSocket6();
        static Socket::ptr CreateUDPSocket6();

        static Socket::ptr CreateUnixTCPSocket();
        static Socket::ptr CreateUnixUDPSocket();

        int64_t getSendTimeout();
        bool setSendTimeout(int64_t v);

        int64_t getRecvTimeout();
        bool setRecvTimeout(int64_t v);

        bool getOption(int level, int option, void* result, size_t &len);
        template <class T>
        bool getOption(int level, int option, T& result) {
            size_t length = sizeof(T);
            return getOption(level, option, &result, &length);
        }
        bool setOption(int level, int option, const void* result, size_t len);
        template<class T>
        bool setOption(int level, int option, const T& value) {
            return setOption(level, option, &value, sizeof(T));
        }

        Socket::ptr accept();
        bool init(int sock);
        bool bind(Address::ptr addr);
        bool connect(Address::ptr, uint64_t timeout_ms = -1);
        bool listen(int backlog = SOMAXCONN);
        bool close();

        int send(const void* buffer, size_t length, int flags = 0);
        int send(const iovec* buffers, size_t length, int flags = 0);
        int sendTo(const void* buffer, size_t length, Address::ptr to, int flags = 0);
        int sendTo(const iovec* buffers, int length, Address::ptr to, int flags = 0);
        int recv(void* buffer, size_t length, int flags = 0);
        int recv(iovec* buffers, size_t length, int flags = 0);
        int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);
        int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

        Address::ptr getRemoteAddress();
        Address::ptr getLocalAddress();

        int getFamily() const { return m_family; }
        int getType() const { return m_type; }
        int getProtocol() const { return m_protocol; }

        bool isConnected() const { return m_isConnected; }
        bool isValid() const;
        int getError();

        std::ostream& dump(std::ostream& os) const;
        int getSocket() const { return m_sock; }

        bool cancelRead();
        bool cancelWrite();
        bool cancelAccept();
        bool cancelAll();
    private:
        void initSock();
        void newSock();
        int m_sock;
        int m_family;
        int m_type;
        int m_protocol;
        bool m_isConnected;

        Address::ptr m_localAddress;
        Address::ptr m_remoteAddress;
    };
}
