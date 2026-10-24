#include "nginxwatch.hpp"

std::pair<int, bool> parseListen(const std::vector<std::string>& args) {
  if (args.empty()) throw std::runtime_error("Empty nginx listen directive");
  std::string address = args.front();
  if (address.compare(0, 5, "unix:") == 0) return {0, false};
  auto colon = address.rfind(':');
  std::string value;
  if (allDigits(address)) value = address;
  else if (colon != std::string::npos && address.back() != ']') value = address.substr(colon + 1);
  else value = "80"; // address-only listens use nginx's default HTTP port
  if (!allDigits(value)) throw std::runtime_error("Unsupported nginx listen: " + address);
  int port;
  try { port = std::stoi(value); }
  catch (const std::exception&) { throw std::runtime_error("Invalid nginx port: " + value); }
  if (port < 1 || port > 65535) throw std::runtime_error("Invalid nginx port: " + value);
  bool ssl = false;
  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i] == "quic") return {0, false}; // HTTP/3 is not a TCP HTTP listener
    if (args[i] == "ssl") ssl = true;
  }
  return {port, ssl};
}
