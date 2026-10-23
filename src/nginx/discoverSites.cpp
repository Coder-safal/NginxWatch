#include "nginxwatch.hpp"

std::set<Site> discoverSites(const std::string& dir, const std::set<std::string>& ignore) {
  std::set<Site> sites;
  for (const auto& entry : fs::directory_iterator(dir)) {
    if (entry.is_symlink() && !fs::exists(entry.path()))
      throw std::runtime_error("Broken nginx symlink: " + entry.path().string());
    if (!entry.is_regular_file()) continue;
    try {
      for (const auto& site : parseSites(readFile(entry.path())))
        if (!ignore.count(site.host)) sites.insert(site);
    } catch (const std::exception& e) {
      throw std::runtime_error(entry.path().string() + ": " + e.what());
    }
  }
  if (sites.empty()) throw std::runtime_error("No testable sites found in " + dir);
  return sites;
}
