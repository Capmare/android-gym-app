// Small test runner. No external library needed.
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <set>
#include <string>

#include "core/exercises.hpp"
#include "core/images.hpp"
#include "core/stats.hpp"
#include "core/store.hpp"
#include "ui/app.hpp"
#include "ui/clock.hpp"

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

void test_images() {
    // A tiny pack made by hand: 2 x 1 pixels, 3 exercises. Only exercise 1 has photos.
    std::vector<std::byte> file;
    const auto put = [&](std::uint32_t v, int bytes) {
        for (int i = 0; i < bytes; ++i) file.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF));
    };
    for (const char c : std::string_view("GYMIMG01")) file.push_back(static_cast<std::byte>(c));
    put(2, 2), put(1, 2), put(3, 4);
    put(0, 4), put(16 + 12, 4), put(0, 4);
    put(0xF800, 2), put(0x07E0, 2);   // photo 0: red, green
    put(0x001F, 2), put(0xFFFF, 2);   // photo 1: blue, white
    const ImagePack pack(file);
    CHECK(pack.exercise_count() == 3);
    CHECK(!pack.has(0) && pack.has(1) && !pack.has(2));
    CHECK(!pack.frame(1, 2) && !pack.frame(3, 0) && !pack.frame(-1, 0));
    const auto img = pack.frame(1, 1);
    CHECK(img && img->width == 2 && img->height == 1 && img->rgb565.size() == 4);

    // Drawing scales the photo: left half red, right half green.
    Canvas c;
    c.resize(8, 2);
    c.draw_image({0, 0, 8, 2}, *pack.frame(1, 0));
    CHECK(c.row(0)[0] == rgb(255, 0, 0) && c.row(1)[7] == rgb(0, 255, 0));

    // Damaged files are refused, not trusted.
    CHECK(ImagePack(std::span<const std::byte>{}).exercise_count() == 0);
    const std::vector<std::byte> junk(100, std::byte{7});
    CHECK(ImagePack(junk).exercise_count() == 0);
    const std::vector<std::byte> cut(file.begin(), file.begin() + 30);
    CHECK(!ImagePack(cut).has(1));  // photo data is cut off

    // The real photo file that goes into the APK.
    const auto real = read_binary_file(GYM_PHOTOS);
    const ImagePack photos(real);
    // The pack lists exercises in the same order. Exercises added later at the end may not have photos yet.
    CHECK(photos.exercise_count() <= static_cast<int>(kExercises.size()) && photos.exercise_count() >= 195);
    int with_photos = 0;
    for (int i = 0; i < photos.exercise_count(); ++i) with_photos += photos.has(i);
    CHECK(with_photos >= 150);
    CHECK(photos.has(0) && photos.has(1) && photos.has(2));
}

void test_store() {
    Data data;
    data.log = {{1700000000, {{0, 600, 5}, {1, 1000, 3}}, {{2, "MARLBORO", false, false}, {1, "GOLDEN VIRGINIA", true, true}}},
                {1700090000, {{2, 1400, 1}}, {}}};
    data.plans = {{"PUSH DAY", {{0, 4, 8}, {3, 3, 10}}}, {"LEG DAY", {{1, 5, 5}}}};
    data.week = {0, -1, 1, -1, 0, -1, -1};
    const auto text = serialize(data);
    const auto back = parse(text);
    CHECK(back.has_value());
    if (back) {
        CHECK(back->log.size() == 2);
        CHECK(back->log[0].sets == data.log[0].sets);
        CHECK(back->log[0].smokes == data.log[0].smokes);   // brand with a space, rolled, after
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
    CHECK(!parse("gym 1\nsmoke 1 0 0 CAMEL\n"));            // smoke before session
    CHECK(!parse("gym 1\nsession 1\nsmoke 0 0 0 CAMEL\n"));  // zero cigarettes
    CHECK(!parse("gym 1\nsession 1\nsmoke 1 2 0 CAMEL\n"));  // flag must be 0 or 1
    CHECK(parse("gym 1\nsession 1\nsmoke 3 1 0 \n").has_value());  // empty brand is allowed
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

    // Photos on the screen: the workout card and the search list.
    const auto real = read_binary_file(GYM_PHOTOS);
    const ImagePack photos(real);
    app.set_images(&photos);
    app.set_tab(Tab::Workout);
    app.frame(canvas, Input{}, t0);
    CHECK(app.animating());                       // the photo moves, so the screen keeps redrawing
    app.frame(canvas, Input{}, t0 + 800ms);       // the other photo
    app.open_picker(false);
    app.frame(canvas, Input{}, t0);
    app.back();
    app.set_images(nullptr);
    app.frame(canvas, Input{}, t0);
    CHECK(!app.animating());

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

    // A tap on the LOG SET button adds a set. On a tall screen (720 x 2400, no scrolling needed)
    // the button is at about y = 1257 to 1385.
    Canvas tall;
    tall.resize(720, 2400);
    app.set_tab(Tab::Workout);
    const auto before = app.log().back().sets.size();
    Input tap{.x = 360, .y = 1300, .press_x = 360, .press_y = 1300, .down = false, .released = true};
    app.frame(tall, tap, t0);
    CHECK(app.log().back().sets.size() == before + 1);

    // A drag that ends on the button is a scroll, not a tap.
    tap.moved = true;
    app.frame(tall, tap, t0);
    CHECK(app.log().back().sets.size() == before + 1);
}


void test_smoking() {
    CHECK(kExercises[static_cast<std::size_t>(kSmoking)].name == "Smoking");
    CHECK(kSmoking == static_cast<int>(kExercises.size()) - 1);   // added at the end: old saved data stays valid

    int saves = 0;
    App app({}, [&](const Data&) { ++saves; }, 2.0f);
    Canvas canvas;
    canvas.resize(720, 2400);
    const auto t0 = App::Clock::now();
    app.frame(canvas, Input{}, t0);

    app.select_exercise(kSmoking);
    CHECK(app.smoking());
    app.set_smoke(3, "MARLBORO", false, false);   // 3 before training, from the pack
    app.log_smoke();
    app.set_smoke(2, "DRUM", true, true);         // 2 after training, self rolled
    app.log_smoke();
    app.set_smoke(1, "", false, true);            // no brand
    app.log_smoke();
    CHECK(app.workout_active() && app.log().size() == 1);
    const auto& smokes = app.log()[0].smokes;
    CHECK(smokes.size() == 3);
    CHECK(smokes[0] == SmokeEntry(3, "MARLBORO", false, false));
    CHECK(smokes[1] == SmokeEntry(2, "DRUM", true, true));
    CHECK(smokes[2].brand == "UNKNOWN");
    CHECK(app.log()[0].sets.empty() && !app.animating());  // no rest timer for smoking
    CHECK(saves == 3);

    // The next time you pick Smoking it starts from the last brand and type.
    app.select_exercise(0);
    app.select_exercise(kSmoking);
    CHECK(last_smoke(app.log())->brand == "UNKNOWN");

    // Tap the brand box (found by trying taps down the screen). The keyboard opens and the box
    // is scrolled into view, above the keyboard.
    {
        Canvas phone;
        phone.resize(720, 1440);
        int opened_at = -1;
        for (int y = 200; y < 1300 && opened_at < 0; y += 12) {
            App a({}, nullptr, 2.0f);
            a.select_exercise(kSmoking);
            a.frame(phone, Input{}, t0);
            a.frame(phone, Input{.x = 360, .y = y, .press_x = 360, .press_y = y, .released = true}, t0);
            if (a.keyboard_open()) opened_at = y;
        }
        CHECK(opened_at > 0);
    }

    // The keyboard opens for the brand box. Every tab still draws.
    app.show_keyboard();
    app.frame(canvas, Input{}, t0);
    CHECK(app.back());                       // first back closes the keyboard
    for (const auto tab : {Tab::Workout, Tab::History, Tab::Records, Tab::Plan}) {
        app.set_tab(tab);
        app.frame(canvas, Input{}, t0);
    }

    // Undo removes smokes one by one on the Smoking card. The empty workout then disappears.
    app.set_tab(Tab::Workout);
    app.undo_set(); app.undo_set();
    CHECK(app.log()[0].smokes.size() == 1);
    app.undo_set();
    CHECK(app.log().empty() && !app.workout_active());
}

void test_time_edit() {
    const std::int64_t now = epoch_now();
    Data data;
    data.log = {{now - 5 * 86400, {{0, 600, 5}}, {}}, {now - 3 * 86400, {{1, 800, 5}}, {}}, {now - 1000, {{2, 900, 3}}, {}}};
    int saves = 0;
    App app(std::move(data), [&](const Data&) { ++saves; }, 2.0f);
    Canvas canvas;
    canvas.resize(720, 2400);
    const auto t0 = App::Clock::now();
    app.frame(canvas, Input{}, t0);
    CHECK(app.workout_active());   // the newest workout is under 6 hours old, so it is still running

    // Move the running workout 4 days back: it lands between the two old ones.
    app.edit_time(2);
    CHECK(app.page() == Page::TimeEdit);
    app.shift_session_days(2, -4);
    CHECK(app.log()[1].sets[0].exercise == 2);        // now in the middle
    CHECK(app.log()[0].sets[0].exercise == 0 && app.log()[2].sets[0].exercise == 1);
    CHECK(app.workout_active());                      // it is still the running workout
    app.log_set();                                    // so a new set goes to that workout
    CHECK(app.log()[1].sets.size() == 2 && app.log()[2].sets.size() == 1);
    CHECK(saves >= 2);

    // The editor page follows the workout and draws.
    app.frame(canvas, Input{}, t0);
    CHECK(app.page() == Page::TimeEdit);

    // The workout we are editing has a set of exercise 2. Find it again after every move, because moves reorder the list.
    const auto where = [&] {
        return static_cast<int>(std::ranges::find_if(app.log(), [](const Session& s) { return s.sets[0].exercise == 2; }) -
                                app.log().begin());
    };

    // Minute steps and a calendar day step.
    const auto before = app.log()[static_cast<std::size_t>(where())].start;
    app.shift_session_minutes(where(), 90);
    CHECK(app.log()[static_cast<std::size_t>(where())].start == before + 90 * 60);
    app.shift_session_days(where(), 1);
    CHECK(format_date(app.log()[static_cast<std::size_t>(where())].start).substr(0, 3) != format_date(before).substr(0, 3));  // another weekday

    // Never in the future.
    app.set_session_start(where(), now + 10 * 86400);
    CHECK(app.log().back().start <= epoch_now());
    CHECK(app.log().back().sets[0].exercise == 2);    // it moved to the end, as the newest

    // Bad numbers do nothing.
    app.edit_time(99);
    app.shift_session_days(-1, 3);

    // Back leaves the page. The History tab has an EDIT button that opens it.
    CHECK(app.back() && app.page() == Page::Main);
    app.set_tab(Tab::History);
    bool opened = false;
    for (int y = 150; y < 2300 && !opened; y += 8) {
        App a(app.data(), nullptr, 2.0f);
        a.set_tab(Tab::History);
        a.frame(canvas, Input{}, t0);
        a.frame(canvas, Input{.x = 640, .y = y, .press_x = 640, .press_y = y, .released = true}, t0);
        opened = a.page() == Page::TimeEdit;
    }
    CHECK(opened);
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
    test_images();
    test_store();
    test_plans();
    test_smoking();
    test_time_edit();
    test_app();
    if (failures == 0) std::puts("all tests passed");
    return failures == 0 ? 0 : 1;
}
