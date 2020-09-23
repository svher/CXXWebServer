#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "webserver.h"

svher::Logger::ptr g_logger = LOG_ROOT();

void test_sleep(){
    svher::IOManager ioManager;
    ioManager.schedule([]() {
        sleep(2);
        LOG_INFO(g_logger) << "sleep 2";
    });

    ioManager.schedule([]() {
        sleep(3);
        LOG_INFO(g_logger) << "sleep 3";
    });
}

void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    // convert values between host and network byte order
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "183.95.80.125", &addr.sin_addr.s_addr);
    int ret = connect(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
    LOG_INFO(g_logger) << "connect ret=" << ret << " errno=" << errno;
    if (ret) return;
    const char data[] = "GET / HTTP 1.1 \r\n\r\n";
    ret = send(sock, data, sizeof(data), 0);
    LOG_INFO(g_logger) << "send ret=" << ret << " errno=" << errno;
    if (ret <= 0) {
        return;
    }
    std::string buff;
    buff.resize(4096);
    ret = recv(sock, &buff[0], buff.size(), 0);
    LOG_INFO(g_logger) << "recv ret=" << ret << " errno=" << errno;
    if (ret < 0) {
        return;
    }
    buff.resize(ret);
    std::cout << buff << std::endl;
}

int main(int argc, char** argv) {
//    test_sleep();
//    test_sock();
    YAML::Node root = YAML::LoadFile("../log.yml");
    svher::Config::LoadFromYaml(root);
    std::cout << svher::LoggerMgr().GetInstance()->toYamlString() << std::endl;
    svher::IOManager ioManager;
    // schedule 相当于创建一个协程，后面异步请求相当于把该协程作为一个工作项
    ioManager.schedule(test_sock);
    return 0;
}