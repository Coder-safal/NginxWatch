#include "nginxwatch.hpp"

bool handleEvent(Event ev, const Site& s, const State& st, const Result& local,
                 const Result& pub, const Config& cfg, bool useLocal, bool usePublic,
                 const std::string& logPath) {
  if (ev == Event::None) return true;
  std::string body = buildAlertBody(s, local, pub, useLocal, usePublic);
  const bool down = ev == Event::WentDown;
  const std::string label = down ? "DOWN" : "RECOVERED";
  if (!down) body += "Was down for about " +
      std::to_string((std::time(nullptr) - st.downSince) / 60) + " minute(s).\n";
  logLine(logPath, "[" + nowString() + "] " + label + " " + siteKey(s) + "\n" + body);
  if (!emailConfigured(cfg)) return true;
  if (sendEmail(cfg, label + ": " + siteKey(s), body)) return true;
  logLine(logPath, "[" + nowString() + "] ERROR email failed; retrying next round");
  return false;
}
