#include "nginxwatch.hpp"

std::vector<Site> parseSites(const std::string& text) {
  auto tokens = tokenizeNginx(text);
  struct Scope {
    std::string kind;
    std::vector<std::string> names;
    std::set<std::pair<int, bool>> listeners;
    bool hasListen = false;
    bool stream = false;
  };
  std::vector<Scope> scopes(1);
  std::vector<std::string> directive;
  std::set<Site> sites;
  for (const auto& t : tokens) {
    if (t.compare(0, 2, "v:") == 0) { directive.push_back(t.substr(2)); continue; }
    if (t == "{") {
      if (directive.empty()) throw std::runtime_error("Unexpected nginx opening brace");
      Scope child;
      child.kind = directive[0];
      child.stream = scopes.back().stream || child.kind == "stream" || child.kind == "mail";
      scopes.push_back(child);
    } else if (t == ";") {
      auto& scope = scopes.back();
      if (!scope.stream && scope.kind == "server" && !directive.empty()) {
        std::vector<std::string> args(directive.begin() + 1, directive.end());
        if (directive[0] == "listen") {
          scope.hasListen = true;
          auto listener = parseListen(args);
          if (listener.first) scope.listeners.insert(listener);
        } else if (directive[0] == "server_name") {
          for (auto name : args) {
            std::transform(name.begin(), name.end(), name.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (isTestableName(name)) scope.names.push_back(name);
          }
        }
      }
    } else if (t == "}") {
      if (scopes.size() == 1 || !directive.empty())
        throw std::runtime_error("Unexpected nginx closing brace or missing semicolon");
      auto scope = scopes.back(); scopes.pop_back();
      if (!scope.stream && scope.kind == "server") {
        if (!scope.hasListen) scope.listeners.insert({80, false});
        for (const auto& name : scope.names)
          for (const auto& listener : scope.listeners)
            sites.insert({name, listener.first, listener.second});
      }
    }
    directive.clear();
  }
  if (scopes.size() != 1 || !directive.empty())
    throw std::runtime_error("Incomplete nginx block or directive");
  return {sites.begin(), sites.end()};
}
