#include "nginxwatch.hpp"

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string part;
  while (std::getline(ss, part, sep)) {
    part = trim(part);
    if (!part.empty()) out.push_back(part);
  }
  return out;
}
