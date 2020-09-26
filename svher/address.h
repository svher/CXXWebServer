#pragma once

#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <map>

namespace svher {
    class IPAddress;
    class Address {
    public:
        typedef std::shared_ptr<Address> ptr;
        static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);
        static bool Lookup(std::vector<Address::ptr>& results, const std::string& host,
                           int family = AF_INET, int type = 0, int protocol = 0);
        static Address::ptr LookupAny(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);
        static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);
        static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& results, int family = AF_INET);
        static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& results, const std::string& iface, int family = AF_INET);
        virtual ~Address() = default;
        int getFamily() const;

        virtual sockaddr * getAddr() const = 0;
        virtual socklen_t getAddrLen() const = 0;

        virtual std::ostream& insert(std::ostream& os) const = 0;
        std::string toString() const;

        bool operator<(const Address& rhs) const;
        bool operator==(const Address& rhs) const;
        bool operator!=(const Address& rhs) const;
    };


    class IPAddress : public Address {
    public:
        typedef std::shared_ptr<IPAddress> ptr;
        static IPAddress::ptr Create(const char* address, uint16_t port = 0);
        virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
        virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
        virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;
        virtual uint32_t getPort() const = 0;
        virtual void setPort(uint16_t v) = 0;
    };

    class IPv4Address : public IPAddress {
    public:
        typedef std::shared_ptr<IPv4Address> ptr;

        static IPv4Address::ptr Create(const char* addr, uint16_t port);

        IPv4Address(const sockaddr_in& addr) : m_addr(addr) {};
        IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

        sockaddr* getAddr() const override;
        socklen_t getAddrLen() const override;
        std::ostream& insert(std::ostream& os) const override;

        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
        IPAddress::ptr networkAddress(uint32_t prefix_len) override;
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;
        uint32_t getPort() const override;
        void setPort(uint16_t v) override;
    private:
        sockaddr_in m_addr;
    };

    class IPv6Address : public IPAddress {
    public:
        typedef std::shared_ptr<IPv6Address> ptr;

        static IPv6Address::ptr Create(const char* addr, uint16_t port);

        IPv6Address();
        IPv6Address(const sockaddr_in6 &addr) : m_addr(addr) {};
        IPv6Address(const uint8_t *address, uint16_t port = 0);

        sockaddr* getAddr() const override;
        socklen_t getAddrLen() const override;
        std::ostream& insert(std::ostream& os) const override;

        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
        IPAddress::ptr networkAddress(uint32_t prefix_len) override;
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;
        uint32_t getPort() const override;
        void setPort(uint16_t v) override;
    private:
        sockaddr_in6 m_addr;
    };

    class UnixAddress : public Address {
    public:
        typedef std::shared_ptr<UnixAddress> ptr;
        UnixAddress();
        explicit UnixAddress(const std::string& path);

        sockaddr* getAddr() const override;
        socklen_t getAddrLen() const override;
        void setAddrLen(socklen_t v);
        std::ostream& insert(std::ostream& os) const override;
    private:
        sockaddr_un m_addr;
        socklen_t m_length;
    };

    class UnknownAddress : public Address {
    public:
        typedef std::shared_ptr<UnknownAddress> ptr;
        UnknownAddress(int family);
        UnknownAddress(const sockaddr& addr);

        sockaddr* getAddr() const override;
        socklen_t getAddrLen() const override;
        std::ostream& insert(std::ostream& os) const override;
    private:
        sockaddr m_addr;
    };
}