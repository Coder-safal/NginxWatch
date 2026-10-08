#include "nginxwatch.hpp"

std::set<std::string> ignoreList(const Config& cfg) {
  auto names = split(cfgGet(cfg, "ignore"), ',');
  std::set<std::string> out;
  for (auto name : names) {
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    out.insert(name);
  }
  return out;
}
