#include "nginxwatch.hpp"

std::string buildMailText(const std::string& from, const std::string& to,
                          const std::string& subject, const std::string& body) {
  return "To: <" + to + ">\r\nFrom: <" + from + ">\r\nSubject: " + subject + "\r\n\r\n" +
         toCrlf(body) + "\r\n";
}
