#include "webserver.h"

static svher::Logger::ptr g_logger = LOG_ROOT();

void test() {
    std::vector<svher::Address::ptr> addrs;
    bool ret = svher::Address::Lookup(addrs, "www.baidu.com:80");
    if (!ret) {
        LOG_ERROR(g_logger) << "lookup failed";
        return;
    }
    for (size_t i = 0; i < addrs.size(); ++i) {
        LOG_INFO(g_logger) << '#' << i << ": " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<svher::Address::ptr, uint32_t>> results;
    bool ret = svher::Address::GetInterfaceAddresses(results);
    if (!ret) {
        LOG_ERROR(g_logger) << "GetInterfaceAddresses failed";
        return;
    }
    for (auto& i : results) {
        LOG_INFO(g_logger) << i.first << " : " << i.second.first->toString() << " prefix_len: " << i.second.second;
    }
}

void test_ipv4() {
    auto addr = svher::IPAddress::Create("127.0.0.8");
    if (addr) {
        LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    test_ipv4();
    return 0;
}