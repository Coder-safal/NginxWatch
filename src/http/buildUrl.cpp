#include "nginxwatch.hpp"

std::string buildUrl(const Site& s) {
  bool defaultPort = (s.https && s.port == 443) || (!s.https && s.port == 80);
  return std::string(s.https ? "https://" : "http://") + s.host +
         (defaultPort ? "" : ":" + std::to_string(s.port)) + "/";
}
