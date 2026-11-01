#include "core/store.hpp"

#include <charconv>
#include <format>
#include <fstream>
#include <sstream>
#include <system_error>
#include <vector>

namespace gym {

namespace {

std::vector<std::string_view> split(std::string_view s, char sep) {
    std::vector<std::string_view> out;
    for (;;) {
        const auto i = s.find(sep);
        out.push_back(s.substr(0, i));
        if (i == std::string_view::npos) break;
        s.remove_prefix(i + 1);
    }
    return out;
}

template <class T>
bool to_int(std::string_view s, T& out) {
    const auto* end = s.data() + s.size();
    const auto [p, ec] = std::from_chars(s.data(), end, out);
    return ec == std::errc{} && p == end;
}

}  // namespace

std::string serialize(const Log& log) {
    std::string out = "gym 1\n";
    for (const auto& session : log) {
        out += std::format("session {}\n", session.start);
        for (const auto& e : session.sets)
            out += std::format("set {} {} {}\n", e.exercise, e.weight_x10, e.reps);
    }
    return out;
}

std::expected<Log, std::string> parse(std::string_view text) {
    Log log;
    bool header = false;
    int line_no = 0;
    for (auto line : split(text, '\n')) {
        ++line_no;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.empty()) continue;

        const auto fail = [&](std::string_view why) {
            return std::unexpected(std::format("line {}: {}", line_no, why));
        };
        const auto words = split(line, ' ');

        if (!header) {
            if (line != "gym 1") return fail("bad header");
            header = true;
        } else if (words[0] == "session" && words.size() == 2) {
            Session s;
            if (!to_int(words[1], s.start)) return fail("bad time");
            log.push_back(std::move(s));
        } else if (words[0] == "set" && words.size() == 4) {
            SetEntry e;
            if (log.empty()) return fail("set before session");
            if (!to_int(words[1], e.exercise) || !to_int(words[2], e.weight_x10) || !to_int(words[3], e.reps))
                return fail("bad number");
            if (e.exercise < 0 || e.exercise >= static_cast<int>(kExercises.size())) return fail("unknown exercise");
            if (e.weight_x10 < 0 || e.reps < 1) return fail("bad value");
            log.back().sets.push_back(e);
        } else {
            return fail("unknown line");
        }
    }
    if (!header) return std::unexpected("empty file");
    return log;
}

bool save_file(const std::filesystem::path& path, const Log& log) {
    // Write to a temporary file first. A crash cannot damage the old file.
    auto tmp = path;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << serialize(log);
        if (!f) return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    return !ec;
}

Log load_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    auto log = parse(ss.str());
    return log ? std::move(*log) : Log{};
}

}  // namespace gym
