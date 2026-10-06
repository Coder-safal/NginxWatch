#include "nginxwatch.hpp"

std::string cfgGet(const Config& c, const std::string& key, const std::string& def) {
  auto it = c.find(key);
  return it == c.end() ? def : it->second;
}
