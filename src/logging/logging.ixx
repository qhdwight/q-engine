export module logging;

import std;

export template<typename... Args>
void log(std::format_string<Args...> const& format, Args&&... args) {
//    std::println(std::cout, format, std::forward<Args>(args)...);
    std::cout << std::format(format, std::forward<Args>(args)...) << std::endl;
}

export void check(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;

    log("Assertion failed at {}:{} in function {}", location.file_name(), location.line(), location.function_name());
    std::terminate();
}
