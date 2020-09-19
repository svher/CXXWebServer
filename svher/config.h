#pragma once

#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <map>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <yaml-cpp/yaml.h>
#include <string>
#include <functional>
#include "log.h"

namespace svher {
    class ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        ConfigVarBase(const std::string& name, const std::string& description = "")
            : m_name(name), m_description(description) {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }
        virtual ~ConfigVarBase() {}
        const std::string& getName() const { return m_name; }
        const std::string& getDescription() const { return m_description; }
        virtual std::string toString() = 0;
        virtual bool fromString(const std::string& val) = 0;
        virtual std::string getTypeName() const = 0;
    protected:
        std::string m_name;
        std::string m_description;
    };


    template<class F, class T>
    class LexicalCast {
    public:
        T operator()(const F& v) {
            return boost::lexical_cast<T>(v);
        }
    };

    template<class T>
    class LexicalCast<std::string, std::vector<T>> {
    public:
        std::vector<T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++) {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::vector<T>, std::string> {
    public:
        std::string operator()(const std::vector<T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();

        }
    };

    template<class T>
    class LexicalCast<std::string, std::list<T>> {
    public:
        std::list<T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++) {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::list<T>, std::string> {
    public:
        std::string operator()(const std::list<T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();

        }
    };

    template<class T>
    class LexicalCast<std::string, std::set<T>> {
    public:
        std::set<T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++) {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::set<T>, std::string> {
    public:
        std::string operator()(const std::set<T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();

        }
    };

    template<class T>
    class LexicalCast<std::string, std::unordered_set<T>> {
    public:
        std::unordered_set<T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++) {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::unordered_set<T>, std::string> {
    public:
        std::string operator()(const std::unordered_set<T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();

        }
    };

    template<class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>> {
    public:
        std::unordered_map<std::string, T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++) {
                ss.str("");
                ss << it->second;
                vec.emplace(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string> {
    public:
        std::string operator()(const std::unordered_map<std::string, T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();

        }
    };

    template<class T>
    class LexicalCast<std::string, std::map<std::string, T>> {
    public:
        std::map<std::string, T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++) {
                ss.str("");
                ss << it->second;
                vec.emplace(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::map<std::string, T>, std::string> {
    public:
        std::string operator()(const std::map<std::string, T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();

        }
    };

    // T operator()(const std::string&)
    // std::string operator(const T&)
    template<class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;
        ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
            :ConfigVarBase(name, description), m_val(default_value) {

        }
        std::string toString() override {
            try {
                return ToStr()(m_val);
            } catch (std::exception& e) {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception" << e.what() << ' ' << typeid(m_val).name();
            }
            return "";
        }
        bool fromString(const std::string& val) override {
            try {
                setValue(FromStr()(val));
            } catch (std::exception& e) {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::fromString exception: " << e.what() << ' ' << val;
                return false;
            }
            return true;
        }
        const T& getValue() const { return m_val; }
        void setValue(const T& val) {
            if (val == m_val)
                return;
            for (auto& i : m_cbs)
                i.second(m_val, val);
            m_val = val;
        }
        std::string getTypeName() const override { return typeid(T).name(); }
        void addListener(uint64_t key, on_change_cb cb) {
            m_cbs[key] = cb;
        }
        void delListener(uint64_t key) {
            m_cbs.erase(key);
        }
        void clearListener() {
            m_cbs.clear();
        }
        on_change_cb getListener(uint64_t key) {
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }
    private:
        T m_val;
        // functional 没有重载 compare，无法查找 functional
        // key, functional
        std::map<uint64_t, on_change_cb > m_cbs;
    };

    class Config {
    public:
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description = "") {
            auto it = GetDatas().find(name);
            if (it != GetDatas().end()) {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp) {
                    LOG_INFO(LOG_ROOT()) << "Lookup name=" << name << " exists";
                    return tmp;
                } else {
                    LOG_ERROR(LOG_ROOT()) << "Lookup name=" << name << " exists but type not " <<it->second->getTypeName();
                    return nullptr;
                }
            }
            if (name.find_first_not_of("abcdedghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
                LOG_ERROR(LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
            auto it = GetDatas().find(name);
            if (it == GetDatas().end()) {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static void LoadFromYaml(const YAML::Node& root);
        static ConfigVarBase::ptr LookupBase(const std::string& name);
    private:
        static ConfigVarMap& GetDatas() {
            static ConfigVarMap m_datas;
            return m_datas;
        }
    };
}