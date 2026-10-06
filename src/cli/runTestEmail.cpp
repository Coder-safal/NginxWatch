#include "nginxwatch.hpp"

int runTestEmail(const Config& cfg) {
  bool ok = sendEmail(cfg, "nginxwatch test email",
                      "If you can read this, email alerts work.\nTime: " + nowString() + "\n");
  std::cout << (ok ? "Test email sent.\n" : "Test email FAILED (see message above).\n");
  return ok ? 0 : 1;
}
