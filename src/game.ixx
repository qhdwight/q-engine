export module game;

import std;

using SteadyClock = std::chrono::steady_clock;
using ClockPoint = std::chrono::time_point<SteadyClock>;
using ClockDelta = ClockPoint::duration;

export enum struct OS {
    Windows,
    Linux,
    macOS,
};

#ifndef NDEBUG
export constexpr bool IS_DEBUG = true;
#else
export constexpr bool IS_DEBUG = false;
#endif
#if defined(_WIN32) || defined(__CYGWIN__)
export constexpr OS OS = OS::Windows;
#elif defined(__linux__)
export constexpr OS OS = OS::Linux;
#elif defined(__APPLE__) && defined(__MACH__)
export constexpr OS OS = OS::macOS;
#else
#error Unknown environment!
#endif

using namespace std::literals;
