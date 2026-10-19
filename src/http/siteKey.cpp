#include "nginxwatch.hpp"

std::string siteKey(const Site& s) {
  return std::string(s.https ? "https://" : "http://") + s.host + ":" + std::to_string(s.port);
}
