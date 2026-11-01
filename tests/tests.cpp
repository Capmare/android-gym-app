// Small test runner. No external library needed.
#include <algorithm>
#include <cstdio>
#include <filesystem>

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

void test_store() {
    Log log{{1700000000, {{0, 600, 5}, {1, 1000, 3}}}, {1700090000, {{2, 1400, 1}}}};
    const auto text = serialize(log);
    const auto back = parse(text);
    CHECK(back.has_value());
    if (back) {
        CHECK(back->size() == 2);
        CHECK((*back)[0].sets == log[0].sets);
        CHECK((*back)[1].start == 1700090000);
    }

    CHECK(!parse("nonsense"));
    CHECK(!parse(""));
    CHECK(!parse("gym 1\nset 0 600 5\n"));                // set before session
    CHECK(!parse("gym 1\nsession 1\nset 99 600 5\n"));    // unknown exercise
    CHECK(!parse("gym 1\nsession 1\nset 0 600 0\n"));     // zero reps
    CHECK(!parse("gym 1\nsession 1\nset 0 abc 5\n"));     // not a number
    CHECK(parse("gym 1\r\nsession 5\r\n").has_value());   // Windows line ends

    const auto path = std::filesystem::temp_directory_path() / "gym_test.log";
    CHECK(save_file(path, log));
    CHECK(load_file(path).size() == 2);
    std::filesystem::remove(path);
    CHECK(load_file(path).empty());  // missing file
}

void test_app() {
    int saves = 0;
    App app({}, [&](const Log&) { ++saves; }, 2.0f);
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
    for (const auto tab : {Tab::Workout, Tab::History, Tab::Records}) {
        app.set_tab(tab);
        app.frame(canvas, Input{}, t0);
        const auto first = canvas.row(0)[0];
        bool varied = false;
        for (int y = 0; y < canvas.height() && !varied; ++y)
            for (const auto px : canvas.row(y))
                if (px != first) { varied = true; break; }
        CHECK(varied);
    }

    // A tap on the LOG SET button adds a set. At this size the button is near y = 1090.
    app.set_tab(Tab::Workout);
    const auto before = app.log().back().sets.size();
    Input tap{.x = 360, .y = 1090, .press_x = 360, .press_y = 1090, .down = false, .released = true};
    app.frame(canvas, tap, t0);
    CHECK(app.log().back().sets.size() == before + 1);

    // A drag that ends on the button is a scroll, not a tap.
    tap.moved = true;
    app.frame(canvas, tap, t0);
    CHECK(app.log().back().sets.size() == before + 1);
}

}  // namespace

int main() {
    test_stats();
    test_store();
    test_app();
    if (failures == 0) std::puts("all tests passed");
    return failures == 0 ? 0 : 1;
}
