#include "nginxwatch.hpp"

Config loadConfig(const std::string& path) {
  Config cfg;
  std::istringstream f(readFile(path));
  const std::set<std::string> allowed = {
    "nginx_dir", "interval_sec", "fails_before_down", "timeout_sec", "check_local",
    "check_public", "local_address", "health_path", "http_ca_file", "ignore", "log_file",
    "smtp_url", "smtp_user", "smtp_pass", "mail_from", "mail_to", "smtp_starttls",
    "smtp_ca_file"
  };
  std::string line;
  int number = 0;
  while (std::getline(f, line)) {
    ++number;
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;
    auto eq = line.find('=');
    std::string key = trim(line.substr(0, eq));
    if (eq == std::string::npos || !allowed.count(key))
      throw std::runtime_error("Invalid setting at " + path + ":" + std::to_string(number));
    if (cfg.count(key)) throw std::runtime_error("Duplicate setting: " + key);
    cfg[key] = trim(line.substr(eq + 1));
  }
  return cfg;
}
