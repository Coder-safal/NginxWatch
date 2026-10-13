#include "nginxwatch.hpp"

void logLine(const std::string& logPath, const std::string& line) {
  std::cout << line << std::endl;
  std::ofstream f(logPath, std::ios::app);
  if (!(f << line << '\n'))
    throw std::runtime_error("Cannot write log file: " + logPath);
}
