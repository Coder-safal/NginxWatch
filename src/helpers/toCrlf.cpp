#include "nginxwatch.hpp"

std::string toCrlf(const std::string& s) {
  std::string out;
  for (char c : s) {
    if (c == '\n') out += "\r\n";
    else if (c != '\r') out += c;
  }
  return out;
}
