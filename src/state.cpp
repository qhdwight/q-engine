#include "state.hpp"

void DiagnosticResource::addFrameTime(clock_delta_t delta) {
    frameTimes[frameTimesIndex++] = delta;
    frameTimesIndex %= frameTimes.size();
    readingCount = std::min(readingCount + 1, frameTimes.size());
}

clock_delta_t DiagnosticResource::getAvgFrameTime() const {
    return std::accumulate(frameTimes.begin(), frameTimes.end(), clock_delta_t::zero()) / readingCount;
}
