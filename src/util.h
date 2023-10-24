#pragma once

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"
#include "spdlog/spdlog.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

template <typename... Ts>
struct Overload : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;

struct Vector2f {
    float x;
    float y;
};

struct Vector2i {
    int32_t x;
    int32_t y;
};

template <class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& s) {
    os << "{";
    bool first = true;
    for (const auto& x : s) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << x;
    }
    return os << "}";
}

template <class T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    os << "{";
    bool first = true;
    for (const auto& x : s) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << x;
    }
    return os << "}";
}

template <class K, class V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m) {
    os << "{";
    bool first = true;
    for (const auto& kv : m) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << kv.first << ": " << kv.second;
    }
    return os << "}";
}

inline void PrintBacktrace() {
    void* array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    // fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
}

template <class T, class U>
void AssertEqual(const T& t, const U& u, const std::string& hint = {}) {
    if (!(t == u)) {
        std::ostringstream os;
        os << "Assertion failed: " << t << " != " << u;
        if (!hint.empty()) {
            os << " hint: " << hint;
        }
        PrintBacktrace();
        throw std::runtime_error(os.str());
    }
}

inline void Assert(bool b, const std::string& hint) {
    AssertEqual(b, true, hint);
}

#define ASSERT_EQUAL(x, y)                                                  \
    {                                                                       \
        std::ostringstream __assert_equal_private_os;                       \
        __assert_equal_private_os << #x << " != " << #y << ", " << __FILE__ \
                                  << ":" << __LINE__;                       \
        AssertEqual(x, y, __assert_equal_private_os.str());                 \
    }

#define ASSERT(x)                                                     \
    {                                                                 \
        if (!bool(x)) {                                               \
            std::ostringstream os;                                    \
            os << #x << " is false, " << __FILE__ << ":" << __LINE__; \
            Assert(x, os.str());                                      \
        }                                                             \
    }

template <typename C, typename K = typename C::key_type,
          typename V = typename C::mapped_type>
inline V& MapGetRef(C& m, const K& key) {
    auto it = m.find(key);
    ASSERT(it != m.end());
    return it->second;
}

template <typename C, typename K = typename C::key_type,
          typename V = typename C::mapped_type>
inline const V& MapGetConstRef(const C& m, const K& key) {
    auto it = m.find(key);
    ASSERT(it != m.end());
    return it->second;
}

template <typename C, typename K = typename C::key_type,
          typename V = typename C::mapped_type>
inline void MapInsert(C& m, const K& key, const V& value) {
    auto [_, insert_res] = m.insert({key, value});
    ASSERT(insert_res);
}

template <typename C, typename K = typename C::key_type>
inline void MapErase(C& m, const K& key) {
    ASSERT(m.erase(key) == 1);
}

template <typename S, typename K = typename S::key_type>
inline void SetErase(S& s, const K& key) {
    ASSERT(s.erase(key) == 1);
}

inline nlohmann::json& JsonGetRef(nlohmann::json& j, const std::string& s) {
    auto it = j.find(s);
    ASSERT(it != j.end());
    return *it;
}

inline const nlohmann::json& JsonGetConstRef(const nlohmann::json& j,
                                             const std::string& s) {
    auto it = j.find(s);
    ASSERT(it != j.end());
    return *it;
}

template <typename T>
inline T JsonGetValue(const nlohmann::json& j, const std::string& key) {
    auto j_value = JsonGetConstRef(j, key);
    return j_value.get<T>();
}

template <typename T>
inline void JsonGetValue(const nlohmann::json& j, const std::string& key,
                         T& dst) {
    auto j_value = JsonGetConstRef(j, key);
    dst = j_value.get<T>();
}

template <typename T>
inline void JsonGetValue(const nlohmann::json& j, const std::string& key,
                         T& dst, const T& _default) {
    auto it = j.find(key);
    if (it == j.end()) {
        dst = _default;
    } else {
        dst = it->get<T>();
    }
}

template <typename T>
inline void JsonSetValue(nlohmann::json& j, const std::string& name, T value) {
    ASSERT(j.find(name) == j.end());
    j[name] = value;
}

// Check requirement, return false on failure
#define REQ_CHECK_EX(x, ex)   \
    {                         \
        if (!(x)) {           \
            SPDLOG_ERROR(ex); \
            return false;     \
        }                     \
    }

#define REQ_CHECK(x)                        \
    {                                       \
        if (!(x)) {                         \
            SPDLOG_ERROR("Req check fail"); \
            return false;                   \
        }                                   \
    }

inline std::string GenLabel(std::string unique, const void* obj,
                            std::string visible = "") {
    std::stringstream ss;
    ss << visible << "##" << unique << "_" << reinterpret_cast<intptr_t>(obj);
    return ss.str();
}

struct TimeIt {
    std::chrono::high_resolution_clock::time_point t0;
    std::function<void(float)> cb;

    TimeIt(std::function<void(float)> callback)
        : t0(std::chrono::high_resolution_clock::now()), cb(callback) {}
    ~TimeIt(void) {
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> ms_double = t1 - t0;

        cb(ms_double.count());
    }
};

inline void JsonLoadFile(const std::string& filename, nlohmann::json& json) {
    std::ifstream f(filename);
    f >> json;
}

inline void JsonSaveFile(const std::string& filename,
                         const nlohmann::json& json) {
    std::ofstream f(filename);
    f << std::setw(4) << json;
}

// inline string Join(const std::vector<std::string>& xs, const std::string
// delim) {
//   return std::accumulate(
//       std::begin(xs), std::end(xs), string(),
//       [&delim] (string &ss, string &s)
//       {
//           return ss.empty() ? s : ss + delim + s;
//       });
// }

template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
                 1;  // Extra space for '\0'
    if (size_s <= 0) {
        throw std::runtime_error("Error during formatting.");
    }
    auto size = static_cast<size_t>(size_s);
    auto buf = std::make_unique<char[]>(size);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                       buf.get() + size - 1);  // We don't want the '\0' inside
}

template <typename EnumT, typename ArrayT>
inline ArrayT::value_type& ArrayGet(ArrayT& arr, EnumT idx) {
    return arr.at(static_cast<size_t>(idx));
}

template <typename T>
inline T SafeAdd(T a, T b) {
    int32_t target = static_cast<int32_t>(a) + b;
    if (target > std::numeric_limits<T>::max()) {
        return std::numeric_limits<T>::max();
    }

    if (target < std::numeric_limits<T>::min()) {
        return std::numeric_limits<T>::min();
    }

    return target;
}
