#include "nginxwatch.hpp"

void runForever(const Config& cfg) {
  const std::string dir = cfgGet(cfg, "nginx_dir", "/etc/nginx/sites-enabled");
  const std::string logPath = cfgGet(cfg, "log_file", "nginxwatch.log");
  const int interval = cfgInt(cfg, "interval_sec", 30);
  const int failsNeeded = cfgInt(cfg, "fails_before_down", 3);
  const bool useLocal = cfgInt(cfg, "check_local", 1) != 0;
  const bool usePublic = cfgInt(cfg, "check_public", 1) != 0;
  std::map<Site, State> states;
  std::map<Site, Event> pending;
  // Fail visibly on an invalid discovery folder at startup.
  discoverSites(dir, ignoreList(cfg));
  logLine(logPath, "[" + nowString() + "] nginxwatch started, watching " + dir);
  if (!emailConfigured(cfg)) logLine(logPath, "Email is disabled; events will be logged only.");
  while (!stopRequested) {
    try {
      const auto sites = discoverSites(dir, ignoreList(cfg));
      for (auto it = states.begin(); it != states.end();) {
        if (!sites.count(it->first)) { pending.erase(it->first); it = states.erase(it); }
        else ++it;
      }
      for (const auto& site : sites) {
        if (stopRequested) break;
        const Result local = useLocal ? httpCheck(site, true, cfg) : Result{};
        const Result pub = usePublic ? httpCheck(site, false, cfg) : Result{};
        State& st = states[site];
        const Event ev = updateState(st, isHealthy(local, pub, useLocal, usePublic), failsNeeded);
        // A new transition supersedes an undelivered event, so stale alerts aren't sent.
        if (ev != Event::None) pending[site] = ev;
        auto it = pending.find(site);
        if (it != pending.end() && handleEvent(it->second, site, st, local, pub,
                                              cfg, useLocal, usePublic, logPath)) pending.erase(it);
      }
    } catch (const std::exception& e) {
      logLine(logPath, "[" + nowString() + "] ERROR " + e.what());
    }
    for (int tick = 0; tick < interval * 10 && !stopRequested; ++tick)
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  logLine(logPath, "[" + nowString() + "] nginxwatch stopped");
}
