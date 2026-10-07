#include "nginxwatch.hpp"

int cfgInt(const Config& c, const std::string& key, int def) {
  auto it = c.find(key);
  if (it == c.end()) return def;
  if (!allDigits(it->second)) throw std::runtime_error(key + " must be an integer");
  try { return std::stoi(it->second); }
  catch (const std::exception&) { throw std::runtime_error(key + " is out of range"); }
}
