#include "nginxwatch.hpp"

std::string buildAlertBody(const Site& s, const Result& local, const Result& pub,
                           bool useLocal, bool usePublic) {
  std::string b = "Site: " + siteKey(s) + "\nTime: " + nowString() + "\n";
  if (useLocal) b += "Local: " + describe(local) + "\n";
  if (usePublic) b += "Public: " + describe(pub) + "\n";
  return b;
}
