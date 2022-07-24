#include "state.hpp"

void DiagnosticResource::addFrameTime(clock_delta_t delta) {
    frameTimes[frameTimesIndex++] = delta;
    frameTimesIndex %= frameTimes.size();
    readingCount = std::min(readingCount + 1, frameTimes.size());
}

clock_delta_t DiagnosticResource::getAvgFrameTime() const {
    clock_delta_t avgFrameTime{};
    for (size_t i = 0; i < readingCount; ++i) {
        avgFrameTime += frameTimes[i];
    }
    return avgFrameTime / readingCount;
}
