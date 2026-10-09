#include "nginxwatch.hpp"

std::string smtpPassword(const Config& c) {
  const char* env = std::getenv("SMTP_PASS");
  return env ? std::string(env) : cfgGet(c, "smtp_pass");
}
