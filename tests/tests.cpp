// Small test runner. No external library needed.
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <set>
#include <string>

#include "core/exercises.hpp"
#include "core/stats.hpp"
#include "core/store.hpp"
#include "ui/app.hpp"

namespace {

int failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
            ++failures;                                                   \
        }                                                                 \
    } while (false)

using namespace gym;
using namespace std::chrono_literals;

// Compile-time checks
static_assert(e1rm_x10({0, 1000, 1}) == 1000);   // 1 rep: the weight itself
static_assert(e1rm_x10({0, 600, 5}) == 700);     // 60 kg x 5 -> 70 kg
static_assert(volume_x10(SetEntry{0, 600, 5}) == 3000);

void test_stats() {
    Session s{1000, {{0, 600, 5}, {0, 650, 3}, {1, 1000, 5}}};
    CHECK(volume_x10(s) == 3000 + 1950 + 5000);

    const auto sums = summarize(s);
    CHECK(sums.size() == 2);
    CHECK(sums[0].exercise == 0 && sums[0].sets == 2);
    CHECK(sums[0].top.weight_x10 == 650);  // 65 x 3 (71.5) beats 60 x 5 (70.0)
    CHECK(sums[1].exercise == 1 && sums[1].sets == 1);

    Log log{s, {2000, {{0, 700, 5}}}};
    const auto bests = personal_bests(log);
    CHECK(bests.size() == 2);
    CHECK(bests.front().exercise == 1);  // squat 100 x 5 = 116.6 is the highest
    CHECK(last_set_for(log, 0) == SetEntry(0, 700, 5));
    CHECK(!last_set_for(log, 7));
    CHECK(kg(625) == "62.5");
    CHECK(kg(0) == "0.0");
}

void test_exercises() {
    CHECK(kExercises.size() >= 190);
    CHECK(kExercises[0].name == "Bench Press");  // the first 12 must never move: saved data uses the position
    CHECK(kExercises[1].name == "Squat");
    CHECK(kExercises[11].name == "Lateral Raise");

    std::set<std::string_view> names;
    for (const auto& e : kExercises) names.insert(e.name);
    CHECK(names.size() == kExercises.size());  // no duplicates

    // Every muscle and every equipment type has at least one exercise.
    for (std::size_t m = 0; m < kMuscleNames.size(); ++m)
        CHECK(std::ranges::any_of(kExercises, [&](const Exercise& e) { return static_cast<std::size_t>(e.muscle) == m; }));
    for (std::size_t q = 0; q < kEquipmentNames.size(); ++q)
        CHECK(std::ranges::any_of(kExercises, [&](const Exercise& e) { return static_cast<std::size_t>(e.equipment) == q; }));

    CHECK(matches(kExercises[0], "bench"));
    CHECK(matches(kExercises[0], "BENCH PRESS"));
    CHECK(matches(kExercises[0], "press bench"));   // word order does not matter
    CHECK(matches(kExercises[0], "chest barbell")); // muscle and equipment count
    CHECK(matches(kExercises[0], ""));
    CHECK(!matches(kExercises[0], "squat"));
}

void test_store() {
    Data data;
    data.log = {{1700000000, {{0, 600, 5}, {1, 1000, 3}}}, {1700090000, {{2, 1400, 1}}}};
    data.plans = {{"PUSH DAY", {{0, 4, 8}, {3, 3, 10}}}, {"LEG DAY", {{1, 5, 5}}}};
    data.week = {0, -1, 1, -1, 0, -1, -1};
    const auto text = serialize(data);
    const auto back = parse(text);
    CHECK(back.has_value());
    if (back) {
        CHECK(back->log.size() == 2);
        CHECK(back->log[0].sets == data.log[0].sets);
        CHECK(back->log[1].start == 1700090000);
        CHECK(back->plans == data.plans);
        CHECK(back->week == data.week);
    }

    // Files from the first version (no plans) still load.
    const auto old = parse("gym 1\nsession 5\nset 0 600 5\n");
    CHECK(old.has_value() && old->plans.empty() && old->week[0] == -1);

    CHECK(!parse("nonsense"));
    CHECK(!parse(""));
    CHECK(!parse("gym 1\nset 0 600 5\n"));                // set before session
    CHECK(!parse("gym 1\nsession 1\nset 999 600 5\n"));  // unknown exercise
    CHECK(!parse("gym 1\nsession 1\nset 0 600 0\n"));     // zero reps
    CHECK(!parse("gym 1\nsession 1\nset 0 abc 5\n"));     // not a number
    CHECK(!parse("gym 1\nitem 0 3 10\n"));                // item before plan
    CHECK(!parse("gym 1\nplan A\nitem 0 0 10\n"));        // zero sets
    CHECK(!parse("gym 1\nday 0 0\n"));                    // day uses a plan that does not exist
    CHECK(!parse("gym 1\nplan A\nday 7 0\n"));           // bad weekday
    CHECK(parse("gym 1\r\nsession 5\r\n").has_value());   // Windows line ends

    const auto path = std::filesystem::temp_directory_path() / "gym_test.log";
    CHECK(save_file(path, data));
    CHECK(load_file(path).log.size() == 2);
    CHECK(load_file(path).plans.size() == 2);
    std::filesystem::remove(path);
    CHECK(load_file(path).log.empty());  // missing file
}

void test_app() {
    int saves = 0;
    App app({}, [&](const Data&) { ++saves; }, 2.0f);
    Canvas canvas;
    canvas.resize(720, 1440);
    const auto t0 = App::Clock::now();
    app.frame(canvas, Input{}, t0);

    CHECK(!app.workout_active());
    app.log_set();
    CHECK(app.workout_active());
    CHECK(app.log().size() == 1 && app.log()[0].sets.size() == 1);
    CHECK(saves == 1);

    app.frame(canvas, Input{}, t0);
    CHECK(app.animating());                        // rest timer runs
    app.frame(canvas, Input{}, t0 + 91s);
    CHECK(!app.animating());                       // and ends after 90 s

    app.set_weight_x10(100000);
    CHECK(app.weight_x10() == 9999);               // clamped
    app.set_reps(0);
    CHECK(app.reps() == 1);

    app.select_exercise(-1);
    CHECK(app.exercise() == static_cast<int>(kExercises.size()) - 1);  // wraps
    app.select_exercise(0);
    CHECK(app.weight_x10() == 200);                // restored from the last logged set

    app.undo_set();
    CHECK(app.log().empty() && !app.workout_active());
    CHECK(saves == 2);

    // Every tab draws without crashing and paints something.
    app.log_set();
    for (const auto tab : {Tab::Workout, Tab::Plan, Tab::History, Tab::Records}) {
        app.set_tab(tab);
        app.frame(canvas, Input{}, t0);
        const auto first = canvas.row(0)[0];
        bool varied = false;
        for (int y = 0; y < canvas.height() && !varied; ++y)
            for (const auto px : canvas.row(y))
                if (px != first) { varied = true; break; }
        CHECK(varied);
    }

    // A tap on the LOG SET button adds a set. At this size the button is near y = 1114.
    app.set_tab(Tab::Workout);
    const auto before = app.log().back().sets.size();
    Input tap{.x = 360, .y = 1114, .press_x = 360, .press_y = 1114, .down = false, .released = true};
    app.frame(canvas, tap, t0);
    CHECK(app.log().back().sets.size() == before + 1);

    // A drag that ends on the button is a scroll, not a tap.
    tap.moved = true;
    app.frame(canvas, tap, t0);
    CHECK(app.log().back().sets.size() == before + 1);
}


void test_plans() {
    int saves = 0;
    App app({}, [&](const Data&) { ++saves; }, 2.0f);
    Canvas canvas;
    canvas.resize(720, 1440);
    const auto t0 = App::Clock::now();
    app.frame(canvas, Input{}, t0);

    CHECK(app.new_plan() == 0);
    CHECK(app.page() == Page::PlanEdit);
    app.add_plan_item(0, 0);
    app.add_plan_item(0, 5);
    app.add_plan_item(0, 9999);  // unknown exercise: ignored
    CHECK(app.data().plans[0].items.size() == 2);
    CHECK(app.new_plan() == 1);

    app.assign_day(0, 0);
    app.assign_day(2, 1);
    app.assign_day(3, 5);   // missing plan: ignored
    app.assign_day(9, 0);   // missing day: ignored
    CHECK(app.data().week[0] == 0 && app.data().week[2] == 1 && app.data().week[3] == -1);

    // Deleting plan 0 clears its days and shifts the later plan down.
    app.delete_plan(0);
    CHECK(app.data().plans.size() == 1);
    CHECK(app.data().week[0] == -1 && app.data().week[2] == 0);
    CHECK(app.page() == Page::Main && app.tab() == Tab::Plan);

    // Typing a plan name with the on-screen keyboard
    app.edit_plan(0);
    app.show_keyboard();
    app.frame(canvas, Input{}, t0);
    for (const auto tab_page : {Page::PlanEdit}) CHECK(app.page() == tab_page);

    // Search page: open, filter, pick an exercise for the logger
    app.set_tab(Tab::Workout);
    app.open_picker(false);
    CHECK(app.page() == Page::Picker);
    app.set_search("lat pulldown");
    app.frame(canvas, Input{}, t0);
    CHECK(app.back());                    // closes the picker
    CHECK(app.page() == Page::Main);
    CHECK(!app.can_go_back() && !app.back());  // nothing left to close: the system handles it

    // Insets (status bar and navigation bar) must not break drawing.
    app.set_insets(130, 90);
    for (const auto tab : {Tab::Workout, Tab::Plan, Tab::History, Tab::Records}) {
        app.set_tab(tab);
        app.frame(canvas, Input{}, t0);
    }
    CHECK(saves > 0);
}

}  // namespace

int main() {
    test_stats();
    test_exercises();
    test_store();
    test_plans();
    test_app();
    if (failures == 0) std::puts("all tests passed");
    return failures == 0 ? 0 : 1;
}
