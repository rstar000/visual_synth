#pragma once

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <map>
#include <unordered_map>
#include <set>
#include <string>
#include <vector>

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "json.hpp"

using namespace std;

template <class T>
ostream& operator << (ostream& os, const vector<T>& s) {
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
ostream& operator << (ostream& os, const set<T>& s) {
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
ostream& operator << (ostream& os, const map<K, V>& m) {
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
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  // fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
}

template<class T, class U>
void AssertEqual(const T& t, const U& u, const string& hint = {}) {
  if (!(t == u)) {
    ostringstream os;
    os << "Assertion failed: " << t << " != " << u;
    if (!hint.empty()) {
       os << " hint: " << hint;
    }
    PrintBacktrace();
    throw runtime_error(os.str());
  }
}

inline void Assert(bool b, const string& hint) {
  AssertEqual(b, true, hint);
}


#define ASSERT_EQUAL(x, y) {                          \
  ostringstream __assert_equal_private_os;            \
  __assert_equal_private_os                           \
    << #x << " != " << #y << ", "                     \
    << __FILE__ << ":" << __LINE__;                   \
  AssertEqual(x, y, __assert_equal_private_os.str()); \
}

#define ASSERT(x) {                     \
  if (!bool(x)) {  \
    ostringstream os;                     \
    os << #x << " is false, "             \
      << __FILE__ << ":" << __LINE__;     \
    Assert(x, os.str());                  \
  } \
}


template<typename C, typename K = typename C::key_type, typename V = typename C::mapped_type>
inline V& MapGetRef(C& m, const K& key) {
  auto it = m.find(key);
  ASSERT(it != m.end());
  return it->second;
}

template<typename C, typename K = typename C::key_type, typename V = typename C::mapped_type>
inline const V& MapGetConstRef(const C& m, const K& key) {
  auto it = m.find(key);
  ASSERT(it != m.end());
  return it->second;
}

template<typename C, typename K = typename C::key_type, typename V = typename C::mapped_type>
inline void MapInsert(C& m, const K& key, const V& value) {
  auto [_, insert_res] = m.insert({key, value});
  ASSERT(insert_res);
}

template<typename C, typename K = typename C::key_type>
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

inline const nlohmann::json& JsonGetConstRef(const nlohmann::json& j, const std::string& s) {
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
inline void JsonGetValue(const nlohmann::json& j, const std::string& key, T& dst) {
  auto j_value = JsonGetConstRef(j, key);
  dst = j_value.get<T>();
}

template <typename T>
inline void JsonGetValue(const nlohmann::json& j, const std::string& key, T& dst, const T& _default) {
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
#define REQ_CHECK_EX(x, ex) {           \
  if (!(x)) { std::cout << ex << std::endl; return false; }     \
}                   

#define REQ_CHECK(x) {           \
  if (!(x)) { std::cout << "Req check fail" << std::endl; return false; }     \
}                   

inline std::string GenLabel(std::string unique, const void* obj, std::string visible = "") {
  std::stringstream ss;
  ss << visible << "##" << unique << "_" << reinterpret_cast<intptr_t>(obj);
  return ss.str();
}

struct TimeIt
{
    chrono::high_resolution_clock::time_point t0;
    function<void(float)> cb;
    
    TimeIt(function<void(float)> callback)
        : t0(chrono::high_resolution_clock::now())
        , cb(callback)
    {
    }
    ~TimeIt(void)
    {
        auto  t1 = chrono::high_resolution_clock::now();
        chrono::duration<float, std::milli> ms_double = t1 - t0;
 
        cb(ms_double.count());
    }
};

// inline string Join(const std::vector<std::string>& xs, const std::string delim) {
//   return std::accumulate(
//       std::begin(xs), std::end(xs), string(),
//       [&delim] (string &ss, string &s)
//       {
//           return ss.empty() ? s : ss + delim + s;
//       });
// }
