#pragma once

#include <curl/curl.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <arpa/inet.h>
#include <cctype>
#include <csignal>
#include <stdexcept>
#include <utility>

namespace fs = std::filesystem;

using Config = std::map<std::string, std::string>;

struct Site {
  std::string host;
  int port = 80;
  bool https = false;
  bool operator<(const Site& o) const;
};

struct Result {
  bool ok = false;
  long status = 0;
  int ms = 0;
  std::string error;
};

struct State {
  bool isDown = false;
  int fails = 0;
  std::time_t downSince = 0;
};

enum class Event { None, WentDown, Recovered };


struct MailBody {
  std::string data;
  size_t pos = 0;
};
extern volatile std::sig_atomic_t stopRequested;

std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, char sep);
bool allDigits(const std::string& s);
std::string nowString();
std::string readFile(const fs::path& p);
void logLine(const std::string& logPath, const std::string& line);
std::string toCrlf(const std::string& s);
Config loadConfig(const std::string& path);
std::string cfgGet(const Config& c, const std::string& key, const std::string& def = "");
int cfgInt(const Config& c, const std::string& key, int def);
std::string smtpPassword(const Config& c);
std::string siteKey(const Site& s);
std::string buildUrl(const Site& s);
size_t discardBody(char*, size_t size, size_t n, void*);
Result httpCheck(const Site& s, bool viaLocalhost, const Config& cfg);
std::string describe(const Result& r);
bool isHealthy(const Result& local, const Result& pub, bool useLocal, bool usePublic);
size_t readMailChunk(char* ptr, size_t size, size_t nmemb, void* userp);
std::string buildMailText(const std::string& from, const std::string& to,
                          const std::string& subject, const std::string& body);
bool emailConfigured(const Config& cfg);
bool sendEmail(const Config& cfg, const std::string& subject, const std::string& body);
Event updateState(State& st, bool healthy, int failsNeeded);
std::string buildAlertBody(const Site& s, const Result& local, const Result& pub,
                           bool useLocal, bool usePublic);
bool handleEvent(Event ev, const Site& s, const State& st, const Result& local,
                 const Result& pub, const Config& cfg, bool useLocal, bool usePublic,
                 const std::string& logPath);
std::set<std::string> ignoreList(const Config& cfg);
int runOnce(const Config& cfg);
void runForever(const Config& cfg);
int runTestEmail(const Config& cfg);
void printUsage();
std::vector<std::string> tokenizeNginx(const std::string& text);
bool isTestableName(const std::string& name);
std::vector<Site> parseSites(const std::string& text);
std::set<Site> discoverSites(const std::string& dir, const std::set<std::string>& ignore);
void validateConfig(const Config& cfg);
void requestStop(int);

std::pair<int, bool> parseListen(const std::vector<std::string>& args);
