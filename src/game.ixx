export module game;

import pch;

import std;

using ns_t = std::chrono::nanoseconds;
using ms_t = std::chrono::duration<double, std::chrono::milliseconds::period>;
using sec_t = std::chrono::duration<double, std::chrono::seconds::period>;
using steady_clock_t = std::chrono::steady_clock;
using clock_point_t = std::chrono::time_point<steady_clock_t>;
using clock_delta_t = clock_point_t::duration;

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
