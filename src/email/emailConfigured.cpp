#include "nginxwatch.hpp"

bool emailConfigured(const Config& cfg) {
  return !cfgGet(cfg, "smtp_url").empty() && !cfgGet(cfg, "mail_to").empty() &&
         !cfgGet(cfg, "mail_from", cfgGet(cfg, "smtp_user")).empty();
}
