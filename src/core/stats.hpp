#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/model.hpp"

namespace gym {

constexpr std::int64_t volume_x10(const SetEntry& s) noexcept {
    return std::int64_t{s.weight_x10} * s.reps;
}

// Estimated one-rep max (Epley formula): weight * (1 + reps / 30).
constexpr int e1rm_x10(const SetEntry& s) noexcept {
    if (s.reps <= 1) return s.weight_x10;
    return static_cast<int>(std::int64_t{s.weight_x10} * (30 + s.reps) / 30);
}

std::int64_t volume_x10(const Session& s) noexcept;

// What you did for one exercise inside one session.
struct ExerciseSummary {
    int exercise{};
    int sets{};
    SetEntry top;  // set with the highest estimated 1RM
};
std::vector<ExerciseSummary> summarize(const Session& s);

// Best set ever for each exercise, highest estimated 1RM first.
std::vector<ExerciseSummary> personal_bests(const Log& log);

std::optional<SetEntry> last_set_for(const Log& log, int exercise);

// "60.0" for 600.
std::string kg(int x10);

}  // namespace gym
