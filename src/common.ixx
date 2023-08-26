export module common;

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

export using u8 = std::uint8_t;
export using u16 = std::uint16_t;
export using u32 = std::uint32_t;
export using u64 = std::uint64_t;
export using i8 = std::int8_t;
export using i16 = std::int16_t;
export using i32 = std::int32_t;
export using i64 = std::int64_t;
export using f32 = float;
export using f64 = double;
