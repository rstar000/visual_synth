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

template <typename T>
struct Param : public BaseParam
{
    Param(std::string keyName, T* valuePtr) 
        : BaseParam(keyName), value{valuePtr} {}

    void Save(nlohmann::json& j) const override
    {
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
