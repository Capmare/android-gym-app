#include "core/exercises.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace gym {

namespace {

std::string lower(std::string_view s) {
    std::string out(s);
    std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

}  // namespace

bool matches(const Exercise& e, std::string_view query) {
    const std::string hay = lower(e.name) + ' ' + lower(name(e.muscle)) + ' ' + lower(name(e.equipment));
    const std::string q = lower(query);
    std::string_view rest = q;
    while (!rest.empty()) {
        const auto sep = rest.find(' ');
        const auto word = rest.substr(0, sep);
        if (!word.empty() && !hay.contains(word)) return false;
        if (sep == std::string_view::npos) break;
        rest.remove_prefix(sep + 1);
    }
    return true;
}

}  // namespace gym
