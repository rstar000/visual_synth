#pragma once
#include <string>
#include "util.h"


struct FloatParam {
    FloatParam(const std::string keyName, const std::string& displayName, float defaultValue)
    void Save(nlohmann::json& j) const;
    void Load(const nlohmann::json& j) const;
    float value;
    
    std::string keyName, displayName;
};