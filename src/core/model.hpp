#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace gym {

// Weights are stored in tenths of a kilogram (600 = 60.0 kg).
// Integers avoid rounding errors and floating point formatting.
struct SetEntry {
    int exercise{};   // index into kExercises (core/exercises.hpp)
    int weight_x10{};
    int reps{};
    friend bool operator==(const SetEntry&, const SetEntry&) = default;
};

// A cigarette entry (the "Smoking" exercise).
struct SmokeEntry {
    int count{1};
    std::string brand;
    bool rolled{false};  // false = from the pack, true = self rolled
    bool after{false};   // false = before training, true = after training
    friend bool operator==(const SmokeEntry&, const SmokeEntry&) = default;
};

struct Session {
    std::int64_t start{};  // Unix time in seconds
    std::vector<SetEntry> sets;
    std::vector<SmokeEntry> smokes;
};

using Log = std::vector<Session>;

// One line of a workout plan: "Bench Press, 4 sets of 8".
struct PlanItem {
    int exercise{};
    int sets{3};
    int reps{10};
    friend bool operator==(const PlanItem&, const PlanItem&) = default;
};

struct Plan {
    std::string name;
    std::vector<PlanItem> items;
    friend bool operator==(const Plan&, const Plan&) = default;
};

// Everything the app saves.
struct Data {
    Log log;
    std::vector<Plan> plans;
    std::array<int, 7> week{-1, -1, -1, -1, -1, -1, -1};  // plan index per weekday, Monday first. -1 = rest day.
};

}  // namespace gym
