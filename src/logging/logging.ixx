export module logging;

import std;

export template<typename... Args>
void log(std::format_string<Args...> const& format, Args&&... args) {
//    std::println(std::cout, format, std::forward<Args>(args)...);
    std::cout << std::format(format, std::forward<Args>(args)...) << std::endl;
}

export void check(bool condition,
                  std::source_location location = std::source_location::current()) {
    if (condition) return;

    log("Assertion failed at {}:{} in function {}",
        location.file_name(), location.line(), location.function_name());
    std::terminate();
}

export template<typename T>
void checkEqual(T const &lhs, T const &rhs,
                 std::source_location location = std::source_location::current()) {
    if (lhs == rhs) return;

    if constexpr (std::formattable<T, char>) {
        log("Assertion failed at {}:{} in function {}: {} != {}",
            location.file_name(), location.line(), location.function_name(),
            lhs, rhs);
    }
    std::terminate();
}

export template<typename T>
void checkIn(T const &lhs, std::unordered_set <T> const &rhs,
              std::source_location location = std::source_location::current()) {
    if (rhs.contains(lhs)) return;

    if constexpr (std::formattable<T, char>) {
        log("Assertion failed at {}:{} in function {}: {} not in {}",
            location.file_name(), location.line(), location.function_name(),
            lhs, rhs);
    }
    std::terminate();
}

