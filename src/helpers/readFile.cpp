#include "nginxwatch.hpp"

std::string readFile(const fs::path& p) {
  std::ifstream f(p);
  if (!f) throw std::runtime_error("Cannot open file: " + p.string());
  std::stringstream ss;
  ss << f.rdbuf();
  if (f.bad()) throw std::runtime_error("Cannot read file: " + p.string());
  return ss.str();
}
