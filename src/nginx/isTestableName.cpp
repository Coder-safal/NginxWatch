#include "nginxwatch.hpp"

bool isTestableName(const std::string& name) {
  if (name.empty() || name.size() > 253 || name == "_" || name == "localhost") return false;
  // Literal DNS names and IPv4 addresses only. Skip wildcards, variables and regexes.
  if (!std::isalnum(static_cast<unsigned char>(name.front()))) return false;
  std::string label;
  for (char c : name) {
    if (c == '.') {
      if (label.empty() || label.size() > 63 || label.front() == '-' || label.back() == '-') return false;
      label.clear();
    } else if (std::isalnum(static_cast<unsigned char>(c)) || c == '-') label += c;
    else return false;
  }
  return label.empty() ? name.back() == '.' :
      label.size() <= 63 && label.front() != '-' && label.back() != '-';
}
