#pragma once
#include <string>
#include <variant>
#include "util.h"

struct BaseParam
{
    BaseParam(std::string keyName) : m_keyName{keyName} {}
    virtual void Save(nlohmann::json& j) const = 0;
    virtual void Load(const nlohmann::json& j) = 0;

protected:
    std::string m_keyName;
};

using ParamPtr = std::shared_ptr<BaseParam>;

template <typename T>
struct Param : public BaseParam
{
    Param(std::string keyName, T* valuePtr) 
        : BaseParam(keyName), value{valuePtr} {}

    virtual ~Param() {}
    void Save(nlohmann::json& j) const override
    {
        SPDLOG_INFO("Save param: {}", m_keyName);
        JsonSetValue<T>(j, m_keyName, *value);
    }

    void Load(const nlohmann::json& j) override
    {
        if (j.contains(m_keyName)) {
            JsonGetValue<T>(j, m_keyName, *value);
        } else {
            *value = T{};
        }
    }

    T* value;
};

template <typename ArrayT, typename = std::enable_if_t<is_std_array<ArrayT>::value>>
struct ArrayParam : public BaseParam 
{
    ArrayParam(std::string keyName, ArrayT* valuePtr) 
        : BaseParam(keyName), value{valuePtr} {}

    virtual ~ArrayParam() {}
    void Save(nlohmann::json& j) const override
    {
        SPDLOG_INFO("Save array param: {}", m_keyName);
        nlohmann::json j_arr = nlohmann::json::array();
        for (size_t i = 0; i < value->size(); ++i) {
            j_arr.push_back(value->at(i));
        }

        JsonSetValue(j, m_keyName, j_arr);
    }

    void Load(const nlohmann::json& j) override
    {
        SPDLOG_INFO("Load array param: {}", m_keyName);
        nlohmann::json const& arr = JsonGetConstRef(j, m_keyName);
        ASSERT(arr.is_array());
        for (size_t i = 0; i < value->size(); ++i) {
            value->at(i) = arr.at(i).get<ArrayT::value_type>();
        }
    }

    ArrayT* value;
};
