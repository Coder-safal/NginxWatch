#include "nginxwatch.hpp"

size_t readMailChunk(char* ptr, size_t size, size_t nmemb, void* userp) {
  MailBody* m = static_cast<MailBody*>(userp);
  size_t room = size * nmemb;
  size_t left = m->data.size() - m->pos;
  size_t n = left < room ? left : room;
  if (n == 0) return 0;
  std::memcpy(ptr, m->data.data() + m->pos, n);
  m->pos += n;
  return n;
}
