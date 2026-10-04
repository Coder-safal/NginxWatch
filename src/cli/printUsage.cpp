#include "nginxwatch.hpp"

void printUsage() {
  std::cout << "Usage: nginxwatch [-c config_file] [--once | --test-email]\n"
               "  (no mode)     run continuously and send email alerts\n"
               "  --once        check sites once; no email (0 healthy, 1 unhealthy)\n"
               "  --test-email  send a test email (0 sent, 1 failed)\n"
               "  -h, --help    show this help\n"
               "  -c, --config  settings file (default: nginxwatch.conf)\n"
               "  exit 2        invalid settings, arguments, or discovery failure\n";
}
