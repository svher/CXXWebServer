#include <unistd.h>
#include <yaml-cpp/yaml.h>
#include <config.h>
#include "webserver.h"

svher::Logger::ptr g_logger = LOG_ROOT();

int count = 0;
svher::Mutex s_mutex;

void fun1() {
    LOG_INFO(g_logger) << "name:" << svher::Thread::GetName() << " this->name: "
                << svher::Thread::GetThis()->getName() << " id: " << svher::GetThreadId()
                << " this.id: " << svher::Thread::GetThis()->getId();
    for (int i = 0; i < 50000000; i++) {
        svher::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while(true) {
        LOG_INFO(g_logger) << "===================";
    }
}

void fun3() {
    while(true) {
        LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxx";
    }
}

int main(int argc, char** argv) {
    std::vector<svher::Thread::ptr> threads;
    YAML::Node root = YAML::LoadFile("../log.yml");
    svher::Config::LoadFromYaml(root);
    for (int i = 0; i < 5; ++i) {
        svher::Thread::ptr thread1(new svher::Thread(&fun2, "name_" + std::to_string(i)));
        svher::Thread::ptr thread2(new svher::Thread(&fun3, "name_" + std::to_string(i)));
        threads.push_back(thread1);
        threads.push_back(thread2);
    }
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i]->join();
    }
    LOG_INFO(g_logger) << "count=" << count;
    return 0;
}