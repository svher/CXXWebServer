#include "svher.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

svher::Logger::ptr g_logger = LOG_ROOT();

void test_fiber() {
    LOG_INFO(g_logger) << "test_fiber";
}

void test1() {
    svher::IOManager iomanager(1, true, "iomanag");
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    // convert values between host and network byte order
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "183.95.80.125", &addr.sin_addr.s_addr);
    connect(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
    iomanager.addEvent(sock, svher::IOManager::WRITE, []() {
        LOG_INFO(g_logger) << "connected";
    });
}

int main(int argc, char** argv) {
    test1();
}
