#include <yaml-cpp/yaml.h>
#include "webserver.h"

svher::ConfigVar<int>::ptr g_int_value_config = svher::Config::Lookup<int>("system.port", (int)8080, "system port");
svher::ConfigVar<std::vector<int>>::ptr g_int_vec_config = svher::Config::Lookup<std::vector<int>>("system.int_vec", std::vector<int>{1, 2}, "system int vector");
svher::ConfigVar<std::list<int>>::ptr g_int_lst_config = svher::Config::Lookup<std::list<int>>("system.int_list", std::list<int>{1, 2}, "system int list");
svher::ConfigVar<std::set<int>>::ptr g_int_set_config = svher::Config::Lookup<std::set<int>>("system.int_set", std::set<int>{1, 2}, "system int set");
svher::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_config = svher::Config::Lookup<std::unordered_set<int>>("system.int_uset", std::unordered_set<int>{1, 2}, "system int uset");
svher::ConfigVar<std::unordered_map<std::string, int>>::ptr g_int_umap_config = svher::Config::Lookup<std::unordered_map<std::string, int>>("system.str_int_umap", std::unordered_map<std::string, int>{
        {"a", 2}, {"b", 3}}, "system int umap");
svher::ConfigVar<std::map<std::string, int>>::ptr g_int_map_config = svher::Config::Lookup<std::map<std::string, int>>("system.str_int_map", std::map<std::string, int>{
        {"a", 2}, {"b", 3}}, "system int map");

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
    YAML::Node root = YAML::LoadFile("../test.yml");
    print_yaml(root, 0);
    LOG_INFO(LOG_ROOT()) << root;
}

void test_config() {
    LOG_INFO(LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    LOG_INFO(LOG_ROOT()) << "before: " << g_int_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for (auto& i : v) { \
            LOG_INFO(LOG_ROOT()) << #prefix " " #name ": " << i; \
        }                       \
        LOG_INFO(LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for (auto& i : v) { \
            LOG_INFO(LOG_ROOT()) << #prefix " " #name ": {" << i.first << " - " << i.second << "}"; \
        } \
        LOG_INFO(LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_config, int_vec, before);
    XX(g_int_lst_config, int_lst, before);
    XX(g_int_set_config, int_set, before);
    XX(g_int_uset_config, int_uset, before);
    XX_M(g_int_map_config, int_map, before);
    XX_M(g_int_umap_config, int_umap, before);

    YAML::Node root = YAML::LoadFile("../test.yml");
    svher::Config::LoadFromYaml(root);

    LOG_INFO(LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    LOG_INFO(LOG_ROOT()) << "after: " << g_int_value_config->toString();
    XX(g_int_vec_config, int_vec, after);
    XX(g_int_lst_config, int_lst, after);
    XX(g_int_set_config, int_set, after);
    XX_M(g_int_map_config, int_map, after);
    XX_M(g_int_umap_config, int_umap, after);
}

class Person {
public:
    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name << " age=" << m_age << " sex=" << m_sex << "]";
        return ss.str();
    }
    std::string m_name;
    int m_age = 0;
    bool m_sex = false;
    bool operator==(const Person& oth) const {
        return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
    }
};

svher::ConfigVar<Person>::ptr g_person = svher::Config::Lookup<Person>("class.person", Person(), "system port");
svher::ConfigVar<std::map<std::string, Person>>::ptr g_person_map = svher::Config::Lookup<std::map<std::string, Person>>("class.map", std::map<std::string, Person>(), "system port");



template<>
class svher::LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class svher::LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

svher::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map =
        svher::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "system person");


void test_class() {
    LOG_INFO(LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();
    LOG_INFO(LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    g_person->addListener([](const Person& old_val, const Person& new_val) {
       LOG_INFO(LOG_ROOT()) << "old_val=" << old_val.toString() << " new_val=" << new_val.toString();
    });

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_var->getValue(); \
        for (auto &i : m) { \
            LOG_INFO(LOG_ROOT()) << prefix <<": " << i.first << " - " << i.second.toString(); \
        } \
        LOG_INFO(LOG_ROOT()) << prefix <<": size=" << m.size(); \
    } \

    XX_PM(g_person_map, "class.map before");
    YAML::Node root = YAML::LoadFile("../test.yml");
    svher::Config::LoadFromYaml(root);

    LOG_INFO(LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");
    LOG_INFO(LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}

void test_log() {
    static svher::Logger::ptr system_log = LOG_NAME("system");
    std::cout << svher::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("../log.yml");
    svher::Config::LoadFromYaml(root);
    LOG_INFO(system_log) << "hello system";
    std::cout << "=================================" << std::endl;
    std::cout << svher::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    system_log->setFormatter("%d - %m%n");
    std::cout << "=================================" << std::endl;
    std::cout << svher::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    LOG_INFO(LOG_ROOT()) << "hello root";
    LOG_INFO(system_log) << "hello system 2";
}

int main(int argc, char** argv) {
    // test_yaml();
    // test_config();
    // test_class();
    test_log();

    svher::Config::Visit([](svher::ConfigVarBase::ptr var) {
        LOG_INFO(LOG_ROOT()) << "name=" << var->getName()
                           << " description=" << var->getDescription()
                           << " typename=" << var->getTypeName()
                           << " value=" << var->toString();
    });
    //auto int_map_config = svher::Config::Lookup<std::unordered_map<std::string, int>>("system.str_int_map", std::unordered_map<std::string, int>{
    //      {"a", 2}, {"b", 3}}, "system int map");
}