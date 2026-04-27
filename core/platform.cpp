#include "core/platform.h"

#include <cstdlib>
#include <cstdint>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#endif

namespace core::platform {

namespace {

#if !defined(_WIN32)
std::string shellQuote(const std::string& value) {
    std::string result = "'";
    for (char ch : value) {
        if (ch == '\'') {
            result += "'\\''";
        } else {
            result.push_back(ch);
        }
    }
    result += "'";
    return result;
}
#endif

} // namespace

bool openUrl(const std::string& url) {
    if (url.empty()) {
        return false;
    }

#if defined(_WIN32)
    HINSTANCE result = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<std::intptr_t>(result) > 32;
#elif defined(__APPLE__)
    const std::string command = "open " + shellQuote(url) + " >/dev/null 2>&1 &";
    return std::system(command.c_str()) == 0;
#else
    const std::string command = "xdg-open " + shellQuote(url) + " >/dev/null 2>&1 &";
    return std::system(command.c_str()) == 0;
#endif
}

} // namespace core::platform
