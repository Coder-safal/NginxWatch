#include "nginxwatch.hpp"

std::vector<std::string> tokenizeNginx(const std::string& text) {
  std::vector<std::string> out;
  std::string word;
  bool started = false;
  char quote = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\\') {
      if (++i == text.size()) throw std::runtime_error("Incomplete nginx escape");
      word += text[i]; started = true;
    } else if (quote) {
      if (c == quote) quote = 0;
      else word += c;
    } else if (c == '$' && i + 1 < text.size() && text[i + 1] == '{') {
      const auto end = text.find('}', i + 2);
      if (end == std::string::npos) throw std::runtime_error("Unclosed nginx variable");
      word += text.substr(i, end - i + 1);
      started = true;
      i = end;
    } else if (c == '\'' || c == '"') {
      quote = c; started = true;
    } else if (c == '#' || std::isspace(static_cast<unsigned char>(c)) ||
               c == ';' || c == '{' || c == '}') {
      if (started) { out.push_back("v:" + word); word.clear(); started = false; }
      if (c == '#') {
        while (i < text.size() && text[i] != '\n') ++i;
      } else if (c == ';' || c == '{' || c == '}') out.emplace_back(1, c);
    } else { word += c; started = true; }
  }
  if (quote) throw std::runtime_error("Unclosed nginx quote");
  if (started) out.push_back("v:" + word);
  return out;
}
