#include "config.h"

namespace svher {
    static void ListAllMember(const std::string& prefix, const YAML::Node& node, std::list<std::pair<std::string, YAML::Node>>& output) {
        if (prefix.find_first_not_of("abcdedghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
            LOG_ERROR(LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.emplace_back(prefix, node);
        if (node.IsMap()) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    void Config::LoadFromYaml(const YAML::Node &root) {
        std::list<std::pair<std::string, YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);

        for (auto &i : all_nodes) {
            std::string key = i.first;
            if (key.empty()) {
                continue;
            }
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = LookupBase(key);

            if (var) {
                if (i.second.IsScalar()) {
                    var->fromString(i.second.Scalar());
                } else {
                    std::stringstream  ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

    ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
        auto it = GetDatas().find(name);
        if (it != GetDatas().end()) {
            return it->second;
        }
        else return nullptr;
    }

    void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap &m = GetDatas();
        for (auto & it : m) {
            cb(it.second);
        }
    }
}