#include "nginxwatch.hpp"

Event updateState(State& st, bool healthy, int failsNeeded) {
  if (healthy) {
    st.fails = 0;
    if (st.isDown) {
      st.isDown = false;
      return Event::Recovered;
    }
    return Event::None;
  }
  if (st.fails < failsNeeded) ++st.fails;
  if (!st.isDown && st.fails >= failsNeeded) {
    st.isDown = true;
    st.downSince = std::time(nullptr);
    return Event::WentDown;
  }
  return Event::None;
}
