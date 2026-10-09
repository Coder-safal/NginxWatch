#include "nginxwatch.hpp"

void validateConfig(const Config& cfg) {
  for (const auto& key : {"interval_sec", "fails_before_down", "timeout_sec"}) {
    const int v = cfgInt(cfg, key, 1);
    if (v < 1 || v > 86400) throw std::runtime_error(std::string(key) + " must be 1..86400");
  }
  for (const auto& key : {"check_local", "check_public", "smtp_starttls"}) {
    const int v = cfgInt(cfg, key, 0);
    if (v != 0 && v != 1) throw std::runtime_error(std::string(key) + " must be 0 or 1");
  }
  if (!cfgInt(cfg, "check_local", 1) && !cfgInt(cfg, "check_public", 1))
    throw std::runtime_error("Enable check_local or check_public");
  const std::string path = cfgGet(cfg, "health_path", "/");
  if (path.empty() || path[0] != '/' || path.find_first_of("\r\n") != std::string::npos)
    throw std::runtime_error("health_path must start with / and contain no newlines");
  const std::string address = cfgGet(cfg, "local_address", "127.0.0.1");
  unsigned char bytes[16];
  if (inet_pton(AF_INET, address.c_str(), bytes) != 1 &&
      inet_pton(AF_INET6, address.c_str(), bytes) != 1)
    throw std::runtime_error("local_address must be a literal IPv4 or IPv6 address (without brackets)");
  for (const auto& key : {"nginx_dir", "log_file"})
    if (cfg.count(key) && cfg.at(key).empty()) throw std::runtime_error(std::string(key) + " cannot be empty");
  const std::string url = cfgGet(cfg, "smtp_url");
  if (!url.empty() || !cfgGet(cfg, "mail_to").empty()) {
    if (!emailConfigured(cfg)) throw std::runtime_error("Incomplete email configuration");
    if (url.compare(0, 8, "smtps://") != 0 &&
        !(url.compare(0, 7, "smtp://") == 0 && cfgInt(cfg, "smtp_starttls", 0)))
      throw std::runtime_error("SMTP requires TLS: use smtps:// or smtp_starttls = 1");
    for (const auto& key : {"mail_from", "mail_to", "smtp_user"})
      if (cfgGet(cfg, key).find_first_of("\r\n<>") != std::string::npos)
        throw std::runtime_error(std::string("Invalid email setting: ") + key);
  }
}
