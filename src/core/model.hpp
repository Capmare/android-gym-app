#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace gym {

struct Exercise {
    std::string_view name;
    std::string_view muscle;
};

inline constexpr auto kExercises = std::to_array<Exercise>({
    {"Bench Press", "Chest"},
    {"Squat", "Legs"},
    {"Deadlift", "Back"},
    {"Overhead Press", "Shoulders"},
    {"Barbell Row", "Back"},
    {"Pull-up", "Back"},
    {"Dip", "Chest"},
    {"Barbell Curl", "Arms"},
    {"Triceps Pushdown", "Arms"},
    {"Leg Press", "Legs"},
    {"Romanian Deadlift", "Legs"},
    {"Lateral Raise", "Shoulders"},
});

// Weights are stored in tenths of a kilogram (600 = 60.0 kg).
// Integers avoid rounding errors and floating point formatting.
struct SetEntry {
    int exercise{};   // index into kExercises
    int weight_x10{};
    int reps{};
    friend bool operator==(const SetEntry&, const SetEntry&) = default;
};

struct Session {
    std::int64_t start{};  // Unix time in seconds
    std::vector<SetEntry> sets;
};

using Log = std::vector<Session>;

}  // namespace gym
