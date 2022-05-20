#include "state.hpp"

void DiagnosticResource::addFrameTime(long long int deltaNs) {
    frameTimesNs[frameTimesIndex++] = deltaNs;
    frameTimesIndex %= frameTimesNs.size();
    readingCount = std::min(readingCount + 1, frameTimesNs.size());
}

double DiagnosticResource::getAvgFrameTime() const {
    double avgFrameTime = 0.0;
    for (size_t i = 0; i < readingCount; ++i) {
        avgFrameTime += static_cast<double>(frameTimesNs[i]) / static_cast<double>(readingCount);
    }
    return avgFrameTime;
}
