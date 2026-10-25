#include "nginxwatch.hpp"

bool Site::operator<(const Site& o) const {
  return std::tie(host, port, https) < std::tie(o.host, o.port, o.https);
}
