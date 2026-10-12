#include "nginxwatch.hpp"

bool sendEmail(const Config& cfg, const std::string& subject, const std::string& body) {
  if (!emailConfigured(cfg)) {
    std::cerr << "Email not configured (smtp_url, mail_from/smtp_user and mail_to required)\n";
    return false;
  }
  const std::string url = cfgGet(cfg, "smtp_url");
  const bool starttls = cfgInt(cfg, "smtp_starttls", 0) != 0;
  if (url.compare(0, 8, "smtps://") != 0 &&
      !(url.compare(0, 7, "smtp://") == 0 && starttls)) {
    std::cerr << "Email requires smtps:// or smtp:// with smtp_starttls = 1\n";
    return false;
  }
  const std::string from = cfgGet(cfg, "mail_from", cfgGet(cfg, "smtp_user"));
  const std::string to = cfgGet(cfg, "mail_to");
  if ((from + to + subject).find_first_of("\r\n") != std::string::npos) {
    std::cerr << "Invalid newline in email header\n"; return false;
  }
  const std::string pass = smtpPassword(cfg);
  MailBody mail{buildMailText(from, to, subject, body), 0};
  CURL* c = curl_easy_init();
  if (!c) return false;
  struct curl_slist* rcpt = curl_slist_append(nullptr, ("<" + to + ">").c_str());
  if (!rcpt) { curl_easy_cleanup(c); return false; }
  curl_easy_setopt(c, CURLOPT_URL, url.c_str());
  curl_easy_setopt(c, CURLOPT_PROTOCOLS_STR, "smtp,smtps");
  const std::string user = cfgGet(cfg, "smtp_user");
  if (!user.empty()) {
    curl_easy_setopt(c, CURLOPT_USERNAME, user.c_str());
    curl_easy_setopt(c, CURLOPT_PASSWORD, pass.c_str());
  }
  curl_easy_setopt(c, CURLOPT_MAIL_FROM, ("<" + from + ">").c_str());
  curl_easy_setopt(c, CURLOPT_MAIL_RCPT, rcpt);
  curl_easy_setopt(c, CURLOPT_READFUNCTION, readMailChunk);
  curl_easy_setopt(c, CURLOPT_READDATA, &mail);
  curl_easy_setopt(c, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(c, CURLOPT_PROXY, "");
  curl_easy_setopt(c, CURLOPT_USE_SSL, static_cast<long>(CURLUSESSL_ALL));
  const std::string ca = cfgGet(cfg, "smtp_ca_file");
  if (!ca.empty()) curl_easy_setopt(c, CURLOPT_CAINFO, ca.c_str());
  const CURLcode code = curl_easy_perform(c);
  if (code != CURLE_OK) std::cerr << "Email failed: " << curl_easy_strerror(code) << '\n';
  curl_easy_cleanup(c);
  curl_slist_free_all(rcpt);
  return code == CURLE_OK;
}
