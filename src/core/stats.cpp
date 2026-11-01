#include "core/stats.hpp"

#include <algorithm>
#include <format>
#include <numeric>

namespace gym {

std::int64_t volume_x10(const Session& s) noexcept {
    return std::accumulate(s.sets.begin(), s.sets.end(), std::int64_t{},
                           [](std::int64_t sum, const SetEntry& e) { return sum + volume_x10(e); });
}

namespace {

void add(std::vector<ExerciseSummary>& out, const SetEntry& e) {
    auto it = std::ranges::find(out, e.exercise, &ExerciseSummary::exercise);
    if (it == out.end()) {
        out.push_back({e.exercise, 1, e});
        return;
    }
    ++it->sets;
    if (e1rm_x10(e) > e1rm_x10(it->top)) it->top = e;
}

}  // namespace

std::vector<ExerciseSummary> summarize(const Session& s) {
    std::vector<ExerciseSummary> out;
    for (const auto& e : s.sets) add(out, e);
    return out;
}

std::vector<ExerciseSummary> personal_bests(const Log& log) {
    std::vector<ExerciseSummary> out;
    for (const auto& session : log)
        for (const auto& e : session.sets) add(out, e);
    std::ranges::stable_sort(out, std::greater{}, [](const ExerciseSummary& s) { return e1rm_x10(s.top); });
    return out;
}

std::optional<SetEntry> last_set_for(const Log& log, int exercise) {
    for (auto s = log.rbegin(); s != log.rend(); ++s)
        for (auto e = s->sets.rbegin(); e != s->sets.rend(); ++e)
            if (e->exercise == exercise) return *e;
    return std::nullopt;
}

std::optional<SmokeEntry> last_smoke(const Log& log) {
    for (auto s = log.rbegin(); s != log.rend(); ++s)
        if (!s->smokes.empty()) return s->smokes.back();
    return std::nullopt;
}

std::string kg(int x10) {
    return std::format("{}.{}", x10 / 10, x10 % 10);
}

}  // namespace gym
