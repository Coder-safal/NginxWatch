#include "nginxwatch.hpp"

volatile std::sig_atomic_t stopRequested = 0;

void requestStop(int) {
  stopRequested = 1;
}
