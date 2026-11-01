#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace gym {

enum class Muscle : std::uint8_t {
    Chest, Back, Shoulders, Biceps, Triceps, Forearms, Abs,
    Quads, Hamstrings, Glutes, Calves, Traps, Cardio, FullBody, Lungs,
};

enum class Equipment : std::uint8_t {
    Barbell, Dumbbell, Machine, Cable, Bodyweight, Kettlebell, Band, Other,
};

inline constexpr std::array<std::string_view, 15> kMuscleNames{
    "Chest", "Back", "Shoulders", "Biceps", "Triceps", "Forearms", "Abs",
    "Quads", "Hamstrings", "Glutes", "Calves", "Traps", "Cardio", "Full Body", "Lungs",
};

inline constexpr std::array<std::string_view, 8> kEquipmentNames{
    "Barbell", "Dumbbell", "Machine", "Cable", "Bodyweight", "Kettlebell", "Band", "Other",
};

constexpr std::string_view name(Muscle m) noexcept { return kMuscleNames[static_cast<std::size_t>(m)]; }
constexpr std::string_view name(Equipment e) noexcept { return kEquipmentNames[static_cast<std::size_t>(e)]; }

struct Exercise {
    std::string_view name;
    Muscle muscle;
    Equipment equipment;
};

namespace detail {
using enum Muscle;
using enum Equipment;

// IMPORTANT: saved workouts store the position in this list.
// Only add new exercises at the END. Never reorder or remove.
inline constexpr auto kTable = std::to_array<Exercise>({
    // The first 12 are the original list. Do not change them.
    {"Bench Press", Chest, Barbell},
    {"Squat", Quads, Barbell},
    {"Deadlift", Back, Barbell},
    {"Overhead Press", Shoulders, Barbell},
    {"Barbell Row", Back, Barbell},
    {"Pull-up", Back, Bodyweight},
    {"Dip", Chest, Bodyweight},
    {"Barbell Curl", Biceps, Barbell},
    {"Triceps Pushdown", Triceps, Cable},
    {"Leg Press", Quads, Machine},
    {"Romanian Deadlift", Hamstrings, Barbell},
    {"Lateral Raise", Shoulders, Dumbbell},

    // Chest
    {"Incline Bench Press", Chest, Barbell},
    {"Decline Bench Press", Chest, Barbell},
    {"Dumbbell Bench Press", Chest, Dumbbell},
    {"Incline Dumbbell Press", Chest, Dumbbell},
    {"Decline Dumbbell Press", Chest, Dumbbell},
    {"Dumbbell Fly", Chest, Dumbbell},
    {"Incline Dumbbell Fly", Chest, Dumbbell},
    {"Cable Fly", Chest, Cable},
    {"Low Cable Fly", Chest, Cable},
    {"High Cable Fly", Chest, Cable},
    {"Pec Deck", Chest, Machine},
    {"Chest Press Machine", Chest, Machine},
    {"Incline Chest Press Machine", Chest, Machine},
    {"Smith Machine Bench Press", Chest, Machine},
    {"Push-up", Chest, Bodyweight},
    {"Incline Push-up", Chest, Bodyweight},
    {"Decline Push-up", Chest, Bodyweight},
    {"Weighted Dip", Chest, Other},
    {"Svend Press", Chest, Other},
    {"Landmine Press", Chest, Barbell},
    {"Band Chest Press", Chest, Band},

    // Back
    {"Chin-up", Back, Bodyweight},
    {"Assisted Pull-up", Back, Machine},
    {"Lat Pulldown", Back, Cable},
    {"Close-Grip Lat Pulldown", Back, Cable},
    {"Straight-Arm Pulldown", Back, Cable},
    {"Seated Cable Row", Back, Cable},
    {"Single-Arm Cable Row", Back, Cable},
    {"One-Arm Dumbbell Row", Back, Dumbbell},
    {"T-Bar Row", Back, Barbell},
    {"Pendlay Row", Back, Barbell},
    {"Meadows Row", Back, Barbell},
    {"Chest-Supported Row", Back, Machine},
    {"Machine Row", Back, Machine},
    {"Inverted Row", Back, Bodyweight},
    {"Back Extension", Back, Bodyweight},
    {"Good Morning", Back, Barbell},
    {"Rack Pull", Back, Barbell},
    {"Sumo Deadlift", Back, Barbell},
    {"Trap Bar Deadlift", Back, Other},
    {"Dumbbell Pullover", Back, Dumbbell},
    {"Muscle-up", Back, Bodyweight},
    {"Band Pull-Apart", Back, Band},

    // Shoulders
    {"Dumbbell Shoulder Press", Shoulders, Dumbbell},
    {"Arnold Press", Shoulders, Dumbbell},
    {"Machine Shoulder Press", Shoulders, Machine},
    {"Push Press", Shoulders, Barbell},
    {"Cable Lateral Raise", Shoulders, Cable},
    {"Front Raise", Shoulders, Dumbbell},
    {"Plate Front Raise", Shoulders, Other},
    {"Rear Delt Fly", Shoulders, Dumbbell},
    {"Reverse Pec Deck", Shoulders, Machine},
    {"Face Pull", Shoulders, Cable},
    {"Upright Row", Shoulders, Barbell},
    {"Pike Push-up", Shoulders, Bodyweight},
    {"Handstand Push-up", Shoulders, Bodyweight},
    {"Band Lateral Raise", Shoulders, Band},

    // Traps
    {"Barbell Shrug", Traps, Barbell},
    {"Dumbbell Shrug", Traps, Dumbbell},
    {"Cable Shrug", Traps, Cable},
    {"Smith Machine Shrug", Traps, Machine},

    // Biceps
    {"EZ-Bar Curl", Biceps, Barbell},
    {"Dumbbell Curl", Biceps, Dumbbell},
    {"Hammer Curl", Biceps, Dumbbell},
    {"Incline Dumbbell Curl", Biceps, Dumbbell},
    {"Concentration Curl", Biceps, Dumbbell},
    {"Spider Curl", Biceps, Dumbbell},
    {"Zottman Curl", Biceps, Dumbbell},
    {"Preacher Curl", Biceps, Barbell},
    {"Machine Preacher Curl", Biceps, Machine},
    {"Cable Curl", Biceps, Cable},
    {"Rope Hammer Curl", Biceps, Cable},
    {"Band Curl", Biceps, Band},

    // Triceps
    {"Rope Pushdown", Triceps, Cable},
    {"Skull Crusher", Triceps, Barbell},
    {"Close-Grip Bench Press", Triceps, Barbell},
    {"JM Press", Triceps, Barbell},
    {"Overhead Triceps Extension", Triceps, Dumbbell},
    {"Cable Overhead Extension", Triceps, Cable},
    {"Triceps Kickback", Triceps, Dumbbell},
    {"Bench Dip", Triceps, Bodyweight},
    {"Diamond Push-up", Triceps, Bodyweight},
    {"Triceps Dip Machine", Triceps, Machine},
    {"Band Pushdown", Triceps, Band},

    // Forearms
    {"Wrist Curl", Forearms, Barbell},
    {"Reverse Wrist Curl", Forearms, Barbell},
    {"Reverse Curl", Forearms, Barbell},
    {"Farmer's Walk", Forearms, Dumbbell},
    {"Dead Hang", Forearms, Bodyweight},
    {"Plate Pinch", Forearms, Other},

    // Abs
    {"Crunch", Abs, Bodyweight},
    {"Sit-up", Abs, Bodyweight},
    {"Decline Sit-up", Abs, Bodyweight},
    {"Bicycle Crunch", Abs, Bodyweight},
    {"V-Up", Abs, Bodyweight},
    {"Leg Raise", Abs, Bodyweight},
    {"Hanging Leg Raise", Abs, Bodyweight},
    {"Hanging Knee Raise", Abs, Bodyweight},
    {"Plank", Abs, Bodyweight},
    {"Side Plank", Abs, Bodyweight},
    {"Hollow Hold", Abs, Bodyweight},
    {"Dead Bug", Abs, Bodyweight},
    {"Mountain Climber", Abs, Bodyweight},
    {"Russian Twist", Abs, Bodyweight},
    {"Flutter Kick", Abs, Bodyweight},
    {"Cable Crunch", Abs, Cable},
    {"Pallof Press", Abs, Cable},
    {"Cable Woodchopper", Abs, Cable},
    {"Ab Crunch Machine", Abs, Machine},
    {"Ab Wheel Rollout", Abs, Other},

    // Quads
    {"Front Squat", Quads, Barbell},
    {"Zercher Squat", Quads, Barbell},
    {"Overhead Squat", Quads, Barbell},
    {"Box Squat", Quads, Barbell},
    {"Smith Machine Squat", Quads, Machine},
    {"Hack Squat", Quads, Machine},
    {"Belt Squat", Quads, Machine},
    {"Leg Extension", Quads, Machine},
    {"Goblet Squat", Quads, Kettlebell},
    {"Bulgarian Split Squat", Quads, Dumbbell},
    {"Lunge", Quads, Dumbbell},
    {"Walking Lunge", Quads, Dumbbell},
    {"Barbell Lunge", Quads, Barbell},
    {"Step-up", Quads, Dumbbell},
    {"Bodyweight Squat", Quads, Bodyweight},
    {"Sissy Squat", Quads, Bodyweight},
    {"Pistol Squat", Quads, Bodyweight},
    {"Wall Sit", Quads, Bodyweight},
    {"Jump Squat", Quads, Bodyweight},

    // Hamstrings
    {"Stiff-Leg Deadlift", Hamstrings, Barbell},
    {"Dumbbell Romanian Deadlift", Hamstrings, Dumbbell},
    {"Single-Leg Romanian Deadlift", Hamstrings, Dumbbell},
    {"Lying Leg Curl", Hamstrings, Machine},
    {"Seated Leg Curl", Hamstrings, Machine},
    {"Nordic Curl", Hamstrings, Bodyweight},
    {"Glute-Ham Raise", Hamstrings, Bodyweight},
    {"Band Leg Curl", Hamstrings, Band},

    // Glutes
    {"Hip Thrust", Glutes, Barbell},
    {"Glute Bridge", Glutes, Bodyweight},
    {"Single-Leg Glute Bridge", Glutes, Bodyweight},
    {"Cable Kickback", Glutes, Cable},
    {"Cable Pull-Through", Glutes, Cable},
    {"Donkey Kick", Glutes, Bodyweight},
    {"Fire Hydrant", Glutes, Bodyweight},
    {"Hip Abduction Machine", Glutes, Machine},
    {"Hip Adduction Machine", Glutes, Machine},
    {"Kettlebell Swing", Glutes, Kettlebell},
    {"Reverse Lunge", Glutes, Dumbbell},
    {"Curtsy Lunge", Glutes, Dumbbell},
    {"Sumo Squat", Glutes, Dumbbell},
    {"Banded Lateral Walk", Glutes, Band},

    // Calves
    {"Standing Calf Raise", Calves, Machine},
    {"Seated Calf Raise", Calves, Machine},
    {"Leg Press Calf Raise", Calves, Machine},
    {"Donkey Calf Raise", Calves, Machine},
    {"Dumbbell Calf Raise", Calves, Dumbbell},
    {"Single-Leg Calf Raise", Calves, Bodyweight},
    {"Tibialis Raise", Calves, Bodyweight},

    // Cardio
    {"Running", Cardio, Bodyweight},
    {"Walking", Cardio, Bodyweight},
    {"Treadmill", Cardio, Machine},
    {"Stationary Bike", Cardio, Machine},
    {"Assault Bike", Cardio, Machine},
    {"Rowing Machine", Cardio, Machine},
    {"Elliptical", Cardio, Machine},
    {"Stair Climber", Cardio, Machine},
    {"Jump Rope", Cardio, Other},
    {"Box Jump", Cardio, Bodyweight},
    {"Swimming", Cardio, Other},
    {"Battle Ropes", Cardio, Other},

    // Full body
    {"Clean", FullBody, Barbell},
    {"Power Clean", FullBody, Barbell},
    {"Hang Clean", FullBody, Barbell},
    {"Snatch", FullBody, Barbell},
    {"Clean and Jerk", FullBody, Barbell},
    {"Thruster", FullBody, Barbell},
    {"Kettlebell Clean", FullBody, Kettlebell},
    {"Kettlebell Snatch", FullBody, Kettlebell},
    {"Turkish Get-up", FullBody, Kettlebell},
    {"Burpee", FullBody, Bodyweight},
    {"Bear Crawl", FullBody, Bodyweight},
    {"Medicine Ball Slam", FullBody, Other},
    {"Sled Push", FullBody, Other},

    // Just for fun. Logged with brand and type instead of weight and reps.
    {"Smoking", Lungs, Other},
});
}  // namespace detail

inline constexpr const auto& kExercises = detail::kTable;

// Position of the Smoking exercise in the list.
constexpr int index_of(std::string_view exercise_name) noexcept {
    for (std::size_t i = 0; i < kExercises.size(); ++i)
        if (kExercises[i].name == exercise_name) return static_cast<int>(i);
    return -1;
}
inline constexpr int kSmoking = index_of("Smoking");
static_assert(kSmoking >= 0);

// True if every word of `query` appears in the name, muscle or equipment (ignoring case).
bool matches(const Exercise& e, std::string_view query);

}  // namespace gym
