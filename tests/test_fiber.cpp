#include <yaml-cpp/yaml.h>
#include "svher.h"

svher::Logger::ptr g_logger = LOG_ROOT();

void func() {
    LOG_INFO(g_logger) << "fiber begin";
    svher::Fiber::YieldToHold();
    LOG_INFO(g_logger) << "fiber end";
}

void test_fiber() {
    auto ptr = svher::Fiber::GetThis();
    svher::Fiber::ptr fiber(new svher::Fiber(func, 0, true));
    LOG_INFO(g_logger) << "first call";
    fiber->call();
    LOG_INFO(g_logger) << "second call";
    fiber->call();
}

int main(int argc, char** argv) {
    svher::Thread::SetName("main");
    YAML::Node root = YAML::LoadFile("../log.yml");
    svher::Config::LoadFromYaml(root);
    std::vector<svher::Thread::ptr> threads;
    for (int i = 0; i < 3; ++i) {
        threads.push_back(svher::Thread::ptr(
                new svher::Thread(&test_fiber, "sub_" + std::to_string(i))));
    }
    for (auto t : threads) {
        t->join();
    }
    LOG_INFO(g_logger) << "main exit";
    return 0;
}