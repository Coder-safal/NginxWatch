#include "nginxwatch.hpp"

size_t discardBody(char*, size_t size, size_t n, void*) { return size * n; }
