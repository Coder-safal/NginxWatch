#include "nginxwatch.hpp"

#include <functional>

int main() {
  int checks = 0;
  auto expect = [&](bool ok, const std::string& label) {
    ++checks;
    if (!ok) throw std::runtime_error("FAILED: " + label);
  };
  auto rejects = [&](const std::function<void()>& action, const std::string& label) {
    bool threw = false;
    try { action(); } catch (const std::exception&) { threw = true; }
    expect(threw, label);
  };
  try {
    expect(trim(" \t hello\r\n") == "hello", "trim whitespace");
    expect(split("a, , b", ',') == std::vector<std::string>({"a", "b"}), "split ignore list");
    expect(allDigits("123") && !allDigits("") && !allDigits("-1"), "decimal validation");
    expect(toCrlf("one\r\ntwo\n") == "one\r\ntwo\r\n", "CRLF normalization");
    const auto sites = parseSites(R"NGINX(
      # server { listen 9999; server_name fake.example; }
      http {
        upstream app { server 127.0.0.1:3000; }
        server {
          listen 80;
          listen [::]:80;
          listen 443 ssl;
          listen 8443 ssl;
          server_name Example.COM "www.example.com" _ *.example.com ~^regex$ $hostname "";
          location / {
            set $text "server { listen 9999; server_name fake.example; } # still a string";
            proxy_pass http://${backend};
          }
        }
        server { listen 80; server_name example.com; }
      }
      stream { server { listen 3306; server_name db.example.com; } }
    )NGINX");
    expect(sites.size() == 6, "all listeners, aliases, lowercase and deduplication");
    std::set<Site> indexed(sites.begin(), sites.end());
    expect(indexed.count({"example.com", 80, false}), "HTTP endpoint");
    expect(indexed.count({"www.example.com", 8443, true}), "nonstandard HTTPS endpoint");
    expect(parseSites("server { server_name default.example; }").front().port == 80,
           "implicit port 80");
    expect(!parseSites("server { listen 443; server_name plain.example; }").front().https,
           "port 443 does not imply TLS");
    expect(parseSites("server { listen unix:/tmp/nginx.sock; server_name unix.example; }").empty(),
           "Unix sockets skipped");
    expect(parseSites("server { listen 443 quic; server_name quic.example; }").empty(),
           "QUIC-only listener skipped");
    expect(parseListen({"127.0.0.1"}).first == 80, "address-only listener");
    expect(parseListen({"[::1]:8080"}).first == 8080, "IPv6 listener");
    expect(parseListen({"0.0.0.0:8443", "ssl"}) == std::make_pair(8443, true), "TLS option");
    expect(parseSites("server { listen 80; server_name 'quoted.example'; set $x 'a\\'b#{}'; }").size() == 1,
           "escaped quote and hash in strings");
    rejects([&] { parseSites("server { server_name a.example;"); }, "unclosed block");
    rejects([&] { parseSites("server { server_name a.example }"); }, "missing semicolon");
    rejects([&] { parseSites("server { server_name \"a.example; }"); }, "unclosed quote");
    rejects([&] { parseListen({"99999999999999999999"}); }, "port overflow");
    rejects([&] { parseListen({"65536"}); }, "port range");
    expect(!isTestableName("bad_name.example") && !isTestableName("-bad.example") &&
           !isTestableName("a..example") && isTestableName("127.0.0.1"), "literal name validation");
    expect(siteKey({"a.example", 443, true}) != siteKey({"a.example", 443, false}),
           "scheme in state identity");
    expect(buildUrl({"a.example", 8443, true}) == "https://a.example:8443/", "URL custom port");
    expect(buildUrl({"a.example", 80, false}) == "http://a.example/", "URL default port");
    State state;
    expect(updateState(state, false, 3) == Event::None && state.fails == 1, "first failure");
    expect(updateState(state, true, 3) == Event::None && state.fails == 0, "failure reset");
    updateState(state, false, 3);
    updateState(state, false, 3);
    expect(updateState(state, false, 3) == Event::WentDown && state.isDown, "DOWN threshold");
    expect(updateState(state, false, 3) == Event::None && state.fails == 3, "no duplicate DOWN");
    expect(updateState(state, true, 3) == Event::Recovered && !state.isDown, "recovery event");
    expect(updateState(state, true, 3) == Event::None, "no duplicate recovery");
    const Result ok{true, 200, 1, ""};
    const Result fail{false, 502, 1, "HTTP 502"};
    expect(isHealthy(ok, fail, true, false), "local-only health");
    expect(isHealthy(fail, ok, false, true), "public-only health");
    expect(!isHealthy(ok, fail, true, true), "both paths required");
    validateConfig({});
    rejects([&] { validateConfig({{"timeout_sec", "0"}}); }, "zero timeout");
    rejects([&] { validateConfig({{"interval_sec", "-1"}}); }, "negative interval");
    rejects([&] { validateConfig({{"fails_before_down", "9999999999999999999"}}); }, "setting overflow");
    rejects([&] { validateConfig({{"check_public", "2"}}); }, "invalid boolean");
    rejects([&] { validateConfig({{"check_local", "0"}, {"check_public", "0"}}); }, "no checks");
    rejects([&] { validateConfig({{"health_path", "health"}}); }, "relative health path");
    rejects([&] { validateConfig({{"local_address", "127.0.0.1,evil"}}); }, "invalid local address");
    rejects([&] { validateConfig({{"smtp_url", "smtp://localhost:25"}, {"mail_to", "a@b"},
                                 {"mail_from", "c@d"}}); }, "plaintext SMTP refused");
    expect(ignoreList({{"ignore", "EXAMPLE.com, test.example"}}).count("example.com"),
           "case-insensitive ignore list");
    const auto mail = buildMailText("a@b", "c@d", "test", "hello\nworld");
    expect(mail.find("Subject: test\r\n\r\nhello\r\nworld\r\n") != std::string::npos,
           "SMTP headers and body");
    MailBody body{mail, 0};
    std::string uploaded;
    char chunk[7];
    size_t n;
    while ((n = readMailChunk(chunk, 1, sizeof(chunk), &body))) uploaded.append(chunk, n);
    expect(uploaded == mail, "SMTP body callback partial reads");
    setenv("SMTP_PASS", "from-environment", 1);
    expect(smtpPassword({{"smtp_pass", "from-file"}}) == "from-environment", "password precedence");
    unsetenv("SMTP_PASS");
    expect(smtpPassword({{"smtp_pass", "from-file"}}) == "from-file", "file password fallback");
    std::cout << checks << " unit checks passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
