#include "nginxwatch.hpp"

std::string describe(const Result& r) {
  if (!r.ok) return "FAIL " + r.error;
  return "OK " + std::to_string(r.status) + " (" + std::to_string(r.ms) + "ms)";
}
