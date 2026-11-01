#include "core/store.hpp"

#include <charconv>
#include <format>
#include <fstream>
#include <sstream>
#include <system_error>
#include <vector>

#include "core/exercises.hpp"

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

// Takes the first space-separated word off the front of `s`.
std::string_view next_word(std::string_view& s) {
    const auto i = s.find(' ');
    const auto word = s.substr(0, i);
    s.remove_prefix(i == std::string_view::npos ? s.size() : i + 1);
    return word;
}

template <class T>
bool to_int(std::string_view s, T& out) {
    const auto* end = s.data() + s.size();
    const auto [p, ec] = std::from_chars(s.data(), end, out);
    return ec == std::errc{} && p == end;
}

}  // namespace

std::string serialize(const Data& data) {
    std::string out = "gym 1\n";
    for (const auto& session : data.log) {
        out += std::format("session {}\n", session.start);
        for (const auto& e : session.sets) out += std::format("set {} {} {}\n", e.exercise, e.weight_x10, e.reps);
        for (const auto& s : session.smokes) out += std::format("smoke {} {} {} {}\n", s.count, int{s.rolled}, int{s.after}, s.brand);
    }
    for (const auto& plan : data.plans) {
        out += std::format("plan {}\n", plan.name);
        for (const auto& i : plan.items) out += std::format("item {} {} {}\n", i.exercise, i.sets, i.reps);
    }
    for (std::size_t d = 0; d < data.week.size(); ++d)
        if (data.week[d] >= 0) out += std::format("day {} {}\n", d, data.week[d]);
    return out;
}

std::expected<Data, std::string> parse(std::string_view text) {
    Data data;
    bool header = false;
    int line_no = 0;
    const int exercise_count = static_cast<int>(kExercises.size());
    for (auto line : split(text, '\n')) {
        ++line_no;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.empty()) continue;

        const auto fail = [&](std::string_view why) { return std::unexpected(std::format("line {}: {}", line_no, why)); };
        const auto words = split(line, ' ');

        if (!header) {
            if (line != "gym 1") return fail("bad header");
            header = true;
        } else if (words[0] == "session" && words.size() == 2) {
            Session s;
            if (!to_int(words[1], s.start)) return fail("bad time");
            data.log.push_back(std::move(s));
        } else if (words[0] == "set" && words.size() == 4) {
            SetEntry e;
            if (data.log.empty()) return fail("set before session");
            if (!to_int(words[1], e.exercise) || !to_int(words[2], e.weight_x10) || !to_int(words[3], e.reps))
                return fail("bad number");
            if (e.exercise < 0 || e.exercise >= exercise_count) return fail("unknown exercise");
            if (e.weight_x10 < 0 || e.reps < 1) return fail("bad value");
            data.log.back().sets.push_back(e);
        } else if (line.starts_with("smoke ")) {
            // smoke <count> <rolled 0/1> <after 0/1> <brand, may have spaces>
            if (data.log.empty()) return fail("smoke before session");
            auto rest = line.substr(6);
            SmokeEntry s;
            int rolled = 0, after = 0;
            if (!to_int(next_word(rest), s.count) || !to_int(next_word(rest), rolled) || !to_int(next_word(rest), after))
                return fail("bad number");
            if (s.count < 1 || s.count > 99 || rolled < 0 || rolled > 1 || after < 0 || after > 1) return fail("bad value");
            s.rolled = rolled == 1;
            s.after = after == 1;
            s.brand = std::string(rest);
            data.log.back().smokes.push_back(std::move(s));
        } else if (line.starts_with("plan ")) {
            data.plans.push_back({std::string(line.substr(5)), {}});
        } else if (words[0] == "item" && words.size() == 4) {
            PlanItem i;
            if (data.plans.empty()) return fail("item before plan");
            if (!to_int(words[1], i.exercise) || !to_int(words[2], i.sets) || !to_int(words[3], i.reps))
                return fail("bad number");
            if (i.exercise < 0 || i.exercise >= exercise_count) return fail("unknown exercise");
            if (i.sets < 1 || i.sets > 99 || i.reps < 1 || i.reps > 99) return fail("bad value");
            data.plans.back().items.push_back(i);
        } else if (words[0] == "day" && words.size() == 3) {
            int d = 0, p = 0;
            if (!to_int(words[1], d) || !to_int(words[2], p)) return fail("bad number");
            if (d < 0 || d > 6 || p < 0) return fail("bad value");
            data.week[static_cast<std::size_t>(d)] = p;
        } else {
            return fail("unknown line");
        }
    }
    if (!header) return std::unexpected("empty file");
    for (const int p : data.week)
        if (p >= static_cast<int>(data.plans.size())) return std::unexpected("day uses a missing plan");
    return data;
}

bool save_file(const std::filesystem::path& path, const Data& data) {
    // Write to a temporary file first. A crash cannot damage the old file.
    auto tmp = path;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << serialize(data);
        if (!f) return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    return !ec;
}

Data load_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    auto data = parse(ss.str());
    return data ? std::move(*data) : Data{};
}

}  // namespace gym
