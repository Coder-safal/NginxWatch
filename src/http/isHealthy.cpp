#include "nginxwatch.hpp"

bool isHealthy(const Result& local, const Result& pub, bool useLocal, bool usePublic) {
  return (!useLocal || local.ok) && (!usePublic || pub.ok);
}
