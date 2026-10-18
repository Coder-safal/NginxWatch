#include "nginxwatch.hpp"

Result httpCheck(const Site& s, bool viaLocalhost, const Config& cfg) {
  Result r;
  CURL* c = curl_easy_init();
  if (!c) { r.error = "curl init failed"; return r; }
  const std::string url = buildUrl(s) + cfgGet(cfg, "health_path", "/").substr(1);
  curl_easy_setopt(c, CURLOPT_URL, url.c_str());
  curl_easy_setopt(c, CURLOPT_TIMEOUT, static_cast<long>(cfgInt(cfg, "timeout_sec", 10)));
  curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(c, CURLOPT_PROTOCOLS_STR, "http,https");
  curl_easy_setopt(c, CURLOPT_USERAGENT, "nginxwatch/1.0");
  curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, discardBody);
  const std::string ca = cfgGet(cfg, "http_ca_file");
  if (!ca.empty()) curl_easy_setopt(c, CURLOPT_CAINFO, ca.c_str());
  struct curl_slist* resolve = nullptr;
  if (viaLocalhost) {
    std::string address = cfgGet(cfg, "local_address", "127.0.0.1");
    if (address.find(':') != std::string::npos) address = "[" + address + "]";
    const std::string rule = s.host + ":" + std::to_string(s.port) + ":" + address;
    resolve = curl_slist_append(nullptr, rule.c_str());
    if (!resolve) { curl_easy_cleanup(c); r.error = "curl resolve allocation failed"; return r; }
    curl_easy_setopt(c, CURLOPT_RESOLVE, resolve);
    // A proxy can bypass CURLOPT_RESOLVE and hide a local failure.
    curl_easy_setopt(c, CURLOPT_PROXY, "");
    curl_easy_setopt(c, CURLOPT_NOPROXY, "*");
  }
  char error[CURL_ERROR_SIZE] = {};
  curl_easy_setopt(c, CURLOPT_ERRORBUFFER, error);
  const CURLcode code = curl_easy_perform(c);
  double secs = 0;
  curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &r.status);
  curl_easy_getinfo(c, CURLINFO_TOTAL_TIME, &secs);
  r.ms = static_cast<int>(secs * 1000);
  if (code == CURLE_OK) {
    r.ok = r.status >= 100 && r.status < 500;
    if (!r.ok) r.error = "HTTP " + std::to_string(r.status);
  } else r.error = error[0] ? error : curl_easy_strerror(code);
  curl_easy_cleanup(c);
  curl_slist_free_all(resolve);
  return r;
}
