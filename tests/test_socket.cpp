#include "webserver.h"

static svher::Logger::ptr g_logger = LOG_ROOT();

void test_socket() {
    svher::IPAddress::ptr addr = svher::Address::LookupAnyIPAddress("www.baidu.com", AF_INET);
    if (addr) {
        LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        LOG_INFO(g_logger) << "get address failed";
        return;
    }
    svher::Socket::ptr sock = svher::Socket::CreateTCP(addr);
    addr->setPort(80);
    if (!sock->connect(addr)) {
        LOG_ERROR(g_logger) << "connect " << addr->toString() << " failed";
        return;
    } else {
        LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }
    const char buff[] = "GET / HTTP/1.1\r\n\r\n";
    int ret = sock->send(buff, sizeof(buff));
    if (ret <= 0) {
        LOG_INFO(g_logger) << "send failed ret=" << ret;
    }
    std::string ans;
    ans.resize(4096);
    ret = sock->recv(&ans[0], ans.size());
    if (ret <= 0) {
        LOG_INFO(g_logger) << "recv failed ret=" << ret;
        return;
    }
    ans.resize(ret);
    LOG_INFO(g_logger) << ans;
}

int main(int argc, char** argv) {
    svher::IOManager ioManager;
    ioManager.schedule(&test_socket);
    return 0;
}