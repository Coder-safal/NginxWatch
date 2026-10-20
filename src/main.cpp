#include "nginxwatch.hpp"

int main(int argc, char** argv) {
  std::string configPath = "nginxwatch.conf";
  std::string mode = "run";
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") { printUsage(); return 0; }
    if ((arg == "-c" || arg == "--config") && i + 1 < argc) configPath = argv[++i];
    else if ((arg == "--once" || arg == "--test-email") && mode == "run") mode = arg;
    else { std::cerr << "Invalid or conflicting argument: " << arg << '\n'; printUsage(); return 2; }
  }
  if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
    std::cerr << "Cannot initialize libcurl\n"; return 2;
  }
  std::signal(SIGINT, requestStop);
  std::signal(SIGTERM, requestStop);
  int code = 0;
  try {
    const Config cfg = loadConfig(configPath);
    validateConfig(cfg);
    if (mode == "--once") code = runOnce(cfg);
    else if (mode == "--test-email") code = runTestEmail(cfg);
    else runForever(cfg);
  } catch (const std::exception& e) { std::cerr << "ERROR: " << e.what() << '\n'; code = 2; }
  curl_global_cleanup();
  return code;
}
