#include "config.h"
#include "log.h"
#include <yaml-cpp/yaml.h>

svher::ConfigVar<int>::ptr g_int_value_config = svher::Config::Lookup<int>("system.port", (int)8080, "system port");

void print_yaml(const YAML::Node& node, int level) {
    if (node.IsScalar()) {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type();
    } else if (node.IsNull()) {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type();
    } else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type();
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type();
            print_yaml(node[i], level + 1);
        }
    }
}


void test_yaml() {
    YAML::Node root = YAML::LoadFile("../log.yaml");
    print_yaml(root, 0);

    LOG_INFO(LOG_ROOT()) << root;
}

int main(int argc, char** argv) {
    test_yaml();
    LOG_INFO(LOG_ROOT()) << g_int_value_config->getValue();
    LOG_INFO(LOG_ROOT()) << g_int_value_config->toString();
}