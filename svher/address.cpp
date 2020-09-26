#include <memory>
#include <sstream>
#include <string>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include "address.h"
#include "endian.h"
#include "log.h"

namespace svher {

    template<class T>
    // 将 T 前 bits 之后的位置 1
    static T CreateMask(uint32_t bits) {
        return (1 << (sizeof(T) * 8 - bits)) - 1;
    }

    template<class T>
    static uint32_t CountBytes(T value) {
        uint32_t result = 0;
        // 抹掉最低位的 1
        for (; value; ++result) {
            value &= value - 1;
        }
        return result;
    }

    static Logger::ptr g_logger = LOG_NAME("sys");

    int Address::getFamily() const {
        return getAddr()->sa_family;
    }

    std::string Address::toString() const {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    bool Address::operator<(const Address &rhs) const {
        socklen_t minLength = std::min(getAddrLen(), rhs.getAddrLen());
        int ret = memcmp(getAddr(), rhs.getAddr(), minLength);
        if (ret < 0) return true;
        else if (ret > 0) return false;
        else return getAddrLen() < rhs.getAddrLen();
    }

    bool Address::operator==(const Address &rhs) const {
        return getAddrLen() == rhs.getAddrLen()
                && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
    }

    bool Address::operator!=(const Address &rhs) const {
        // 利用上面的函数
        return !(*this == rhs);
    }

    Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen) {
        if (addr == nullptr) return nullptr;
        Address::ptr result;
        switch (addr->sa_family) {
            case AF_INET:
                result.reset(new IPv4Address(*(const sockaddr_in*)addr));
                break;
            case AF_INET6:
                result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
                break;
            default:
                result.reset(new UnknownAddress(*(const sockaddr*)addr));
                break;
        }
        return result;
    }

    bool Address::Lookup(std::vector<Address::ptr> &results, const std::string &host, int family, int type, int protocol) {
        addrinfo hints, *ans;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        std::string node;
        const char* service = nullptr;
        if (!host.empty() && host[0] == '[') {
            // The memchr() and memrchr() functions return a pointer to the matching byte
            const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
            if (endipv6) {
                // TODO out of range
                if (*(endipv6 + 1) == ':') {
                    // service name -> port number
                    service = endipv6 + 2;
                }
                node = host.substr(1, endipv6 - host.c_str() - 1);
            }
        }
        if (node.empty()) {
            service = (const char*)memchr(host.c_str(), ':', host.size());
            if (service) {
                if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                    node = host.substr(0, service - host.c_str());
                    ++service;
                }
            }
        }
        // 如果找不到端口号
        if (node.empty()) {
            node = host;
        }
        int error = getaddrinfo(node.c_str(), service, &hints, &ans);
        if (error) {
            LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host
                    << ", " << family << ", " << type << ") err=" << error
                    << " errstr=" << strerror(errno);
            return false;
        }
        auto next = ans;
        while (next) {
            results.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
            next = next->ai_next;
        }
        freeaddrinfo(ans);
        return true;
    }

    Address::ptr Address::LookupAny(const std::string &host, int family, int type, int protocol) {
        std::vector<Address::ptr> results;
        if (Lookup(results, host, family, type, protocol)) {
            return results[0];
        }
        return nullptr;
    }

    IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host, int family, int type, int protocol) {
        std::vector<Address::ptr> results;
        if (Lookup(results, host, family, type, protocol)) {
            for (auto& i : results) {
                IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
                if (v) return v;
            }
        }
        return nullptr;
    }

    bool
    Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &results, int family) {
        struct ifaddrs* next, *ans;
        if (getifaddrs(&ans) != 0) {
            LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddrs err="
                    << errno << " errstr=" << strerror(errno);
        }
        try {
            for (next = ans; next; next = next->ifa_next) {
                Address::ptr addr;
                uint32_t prefix_len = 0u;
                if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                    continue;
                }
                switch (next->ifa_addr->sa_family) {
                    case AF_INET: {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBytes(netmask);
                    }
                    case AF_INET6: {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for (int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    default: break;
                }
                if (addr) {
                    results.emplace(next->ifa_name, std::make_pair(addr, prefix_len));
                }
            }
        } catch(...) {
            LOG_ERROR(g_logger) << "Address::GetInterfaceAddress exception";
            freeifaddrs(ans);
            return false;
        }
        freeifaddrs(ans);
        return true;
    }

    bool
    Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &results, const std::string &iface,
                                   int family) {
        if (iface.empty() || iface == "*") {
            if (family == AF_INET || family == AF_UNSPEC) {
                results.emplace_back(Address::ptr(new IPv4Address()), 0);
            }
            if (family == AF_INET6 || family == AF_UNSPEC) {
                results.emplace_back(Address::ptr(new IPv6Address), 0);
            }
            return true;
        }

        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> ans;
        if (!GetInterfaceAddresses(ans, family)) {
            return false;
        }
        auto its = ans.equal_range(iface);
        for(; its.first != its.second; ++its.first) {
            results.push_back(its.first->second);
        }

        return true;
    }

    IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = byteswap_t(port);
        m_addr.sin_addr.s_addr = byteswap_t(address);
    }

    sockaddr *IPv4Address::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    socklen_t IPv4Address::getAddrLen() const {
        return sizeof(sockaddr);
    }

    std::ostream &IPv4Address::insert(std::ostream &os) const {
        uint32_t addr = byteswap_t(m_addr.sin_addr.s_addr);
        os << ((addr >> 24) & 0xff) << "."
            << ((addr >> 16) & 0xff) << "."
            << ((addr >> 8) & 0xff) << "."
            << (addr & 0xff);
        if (m_addr.sin_port)
            os << ':' << byteswap_t(m_addr.sin_port);
        return os;
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
        if (prefix_len > 32) return nullptr;
        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr |= byteswap_t(
                CreateMask<uint32_t>(prefix_len));
        return std::make_shared<IPv4Address>(baddr);
    }

    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
        if (prefix_len > 32) return nullptr;
        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr &= ~byteswap_t(
                CreateMask<uint32_t>(prefix_len));
        return std::make_shared<IPv4Address>(baddr);
    }

    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
        sockaddr_in saddr;
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = ~byteswap_t(CreateMask<uint32_t>(prefix_len));
        return std::make_shared<IPv4Address>(saddr);
    }

    uint32_t IPv4Address::getPort() const {
        return byteswap_t(m_addr.sin_port);
    }

    void IPv4Address::setPort(uint16_t v) {
        m_addr.sin_port = byteswap_t(v);
    }

    IPv4Address::ptr IPv4Address::Create(const char *addr, uint16_t port) {
        IPv4Address::ptr ret(new IPv4Address);
        ret->m_addr.sin_port = byteswap_t(port);
        int result = inet_pton(AF_INET, addr, &ret->m_addr.sin_addr);
        if (result <= 0) {
            LOG_ERROR(g_logger) << "IPv4Address:Create(" << addr
                << ", " << port << " ret=" << result << " errno=" << errno
                << " errstr" << strerror(errno);
            return nullptr;
        }
        return ret;
    }

    IPv6Address::IPv6Address() {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
    }

    IPv6Address::IPv6Address(const uint8_t *address, uint16_t port) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = byteswap_t(port);
        memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
    }

    sockaddr *IPv6Address::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    socklen_t IPv6Address::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream &IPv6Address::insert(std::ostream &os) const {
        os << "[";
        auto* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
        bool used_zeros = false;
        for (int i = 0; i < 8; ++i) {
            // 当 i > 0 时，输出 addr[i]，若遇到 0 跳过
            if (addr[i] == 0 && !used_zeros) {
                continue;
            }
            // 如果当前不为 0，前一个块为 0，则补输出一个前导分隔符
            if (i && addr[i - 1] == 0 && !used_zeros) {
                os << ":";
                used_zeros = true;
            }
            if (i) {
                os << ":";
            }
            os << std::hex << (int)(byteswap_t(addr[i])) << std::dec;
        }
        if (!used_zeros && addr[7] == 0) {
            os << "::";
        }
        os << ']';
        if (m_addr.sin6_port)
            os << ':' << byteswap_t(m_addr.sin6_port);
        return os;
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
        for (int i = prefix_len / 8 + 1; i < 16; ++i) {
            baddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return std::make_shared<IPv6Address>(baddr);
    }

    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] &= ~CreateMask<uint8_t>(prefix_len % 8);
        for (int i = prefix_len / 8 + 1; i < 16; ++i) {
            baddr.sin6_addr.s6_addr[i] = 0;
        }
        return std::make_shared<IPv6Address>(baddr);
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
        sockaddr_in6 saddr;
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin6_family = AF_INET6;
        saddr.sin6_addr.s6_addr[prefix_len / 8] &= ~CreateMask<uint8_t>(prefix_len % 8);
        for (uint32_t i = 0; i < prefix_len / 8; i++) {
            saddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return std::make_shared<IPv6Address>(saddr);
    }

    uint32_t IPv6Address::getPort() const {
        return byteswap_t(m_addr.sin6_port);
    }

    void IPv6Address::setPort(uint16_t v) {
        m_addr.sin6_port = byteswap_t(v);
    }

    IPv6Address::ptr IPv6Address::Create(const char *addr, uint16_t port) {
        IPv6Address::ptr ret(new IPv6Address);
        ret->m_addr.sin6_port = byteswap_t(port);
        int result = inet_pton(AF_INET6, addr, &ret->m_addr.sin6_addr);
        if (result <= 0) {
            LOG_ERROR(g_logger) << "IPv6Address:Create(" << addr
                                << ", " << port << " ret=" << result << " errno=" << errno
                                << " errstr" << strerror(errno);
            return nullptr;
        }
        return svher::IPv6Address::ptr();
    }

    // -1 因为 0 地址已经占用了 1 空间
    const static size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)nullptr)->sun_path) - 1;

    UnixAddress::UnixAddress() {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
    }

    UnixAddress::UnixAddress(const std::string &path) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = path.size() + 1;
        if (!path.empty() && path[0] == '\0') {
            --m_length;
        }
        if (m_length <= sizeof(m_addr.sun_path)) {
            throw std::logic_error("path too long");
        }
        memcpy(&m_addr.sun_path, path.c_str(), m_length);
        m_length += offsetof(sockaddr_un, sun_path);
    }

    sockaddr *UnixAddress::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    socklen_t UnixAddress::getAddrLen() const {
        return m_length;
    }

    std::ostream &UnixAddress::insert(std::ostream &os) const {
        if (m_length > offsetof(sockaddr_un, sun_path)
                && m_addr.sun_path[0] == '\0') {
            return os << "\\0" << std::string(m_addr.sun_path + 1,
                              m_length - offsetof(sockaddr_un, sun_path) - 1);
        }
        return os << m_addr.sun_path;
    }

    void UnixAddress::setAddrLen(socklen_t v) {
        m_length = v;
    }


    UnknownAddress::UnknownAddress(int family) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sa_family = family;
    }

    sockaddr *UnknownAddress::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    socklen_t UnknownAddress::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream &UnknownAddress::insert(std::ostream &os) const {
        os << "[UnknownAddress family=" << m_addr.sa_family << "]";
        return os;
    }

    UnknownAddress::UnknownAddress(const sockaddr &addr) {
        m_addr = addr;
    }

    IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
        addrinfo hints, *results;
        memset(&hints, 0, sizeof(hints));
//        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = AF_UNSPEC;

        int ret = getaddrinfo(address, NULL, &hints, &results);
        if (ret) {
            LOG_ERROR(g_logger) << "IPAddress:Create(" << address
                                << ", " << port << ") ret=" << ret
                                << " errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }
        try {
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                    Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if (result) {
                result->setPort(port);
            }
            freeaddrinfo(results);
            return result;
        } catch (...) {
            freeaddrinfo(results);
        }
        return nullptr;
    }
}