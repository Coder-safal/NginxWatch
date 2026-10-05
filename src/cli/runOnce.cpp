#include "nginxwatch.hpp"

int runOnce(const Config& cfg) {
  const std::string dir = cfgGet(cfg, "nginx_dir", "/etc/nginx/sites-enabled");
  const bool useLocal = cfgInt(cfg, "check_local", 1) != 0;
  const bool usePublic = cfgInt(cfg, "check_public", 1) != 0;
  const auto sites = discoverSites(dir, ignoreList(cfg));
  std::cout << "Found " << sites.size() << " site(s) in " << dir << "\n\n";
  std::cout << std::left << std::setw(48) << "SITE" << std::setw(48) << "LOCAL" << "PUBLIC\n";
  int bad = 0;
  for (const auto& site : sites) {
    const Result local = useLocal ? httpCheck(site, true, cfg) : Result{};
    const Result pub = usePublic ? httpCheck(site, false, cfg) : Result{};
    if (!isHealthy(local, pub, useLocal, usePublic)) ++bad;
    std::cout << std::left << std::setw(48) << siteKey(site) << std::setw(48)
              << (useLocal ? describe(local) : "(skipped)")
              << (usePublic ? describe(pub) : "(skipped)") << '\n';
  }
  std::cout << '\n' << bad << " site(s) with problems\n";
  return bad ? 1 : 0;
}
