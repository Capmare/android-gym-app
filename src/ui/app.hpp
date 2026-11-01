#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "core/exercises.hpp"
#include "core/images.hpp"
#include "core/model.hpp"
#include "ui/canvas.hpp"

namespace gym {

// One touch gesture, as the platform sees it. The platform fills this in.
struct Input {
    int x{}, y{};                  // finger now
    int press_x{}, press_y{};      // where the finger first touched
    bool down{};                   // finger is on the screen
    bool released{};               // finger lifted since the last frame
    bool moved{};                  // finger travelled too far to be a tap
    int drag_dy{};                 // vertical travel since the last frame
};

enum class Tab { Workout, Plan, History, Records };
enum class Page { Main, Picker, PlanEdit };  // Picker and PlanEdit cover the whole screen

class App {
public:
    using Clock = std::chrono::steady_clock;
    using SaveFn = std::function<void(const Data&)>;

    // `unit` is the screen density factor: 1.0 = 160 dpi, 2.0 = 320 dpi.
    App(Data data, SaveFn save, float unit);

    // Draw one frame. May change the state if the user taps a button.
    void frame(Canvas& canvas, const Input& in, Clock::time_point now);

    // True while the screen must repaint without any touch (rest timer).
    bool animating() const noexcept { return rest_end_.has_value() || photo_animating_; }

    // Exercise photos. The pack must stay alive while the app uses it. nullptr = no photos.
    void set_images(const ImagePack* images) noexcept {
        images_ = images;
        dirty_ = true;
    }

    // Size in pixels of the status bar and the navigation bar. The app keeps out of them.
    void set_insets(int top, int bottom) noexcept;

    // Android back button. back() returns true if the app used it.
    bool can_go_back() const noexcept { return page_ != Page::Main || focus_ != Focus::None; }
    bool back();

    // Workout actions. The buttons call these. Tests can call them too.
    void select_exercise(int index);  // wraps around
    void log_set();
    void undo_set();
    void finish_workout();
    void set_weight_x10(int v);
    void set_reps(int v);
    void set_tab(Tab t) noexcept;

    // Smoking (a joke exercise): what to log, and the button that logs it.
    void set_smoke(int count, std::string brand, bool rolled, bool after);
    void log_smoke();

    // Plan actions
    int new_plan();                    // adds a plan, opens the editor, returns its index
    void edit_plan(int index);
    void delete_plan(int index);
    void assign_day(int day, int plan);  // day 0 = Monday. plan -1 = rest day.
    void add_plan_item(int plan, int exercise);

    // Exercise search
    void open_picker(bool for_plan);
    void set_search(std::string text);
    void show_keyboard();

    const Data& data() const noexcept { return data_; }
    const Log& log() const noexcept { return data_.log; }
    Tab tab() const noexcept { return tab_; }
    Page page() const noexcept { return page_; }
    int exercise() const noexcept { return exercise_; }
    int weight_x10() const noexcept { return weight_x10_; }
    int reps() const noexcept { return reps_; }
    bool workout_active() const noexcept { return active_.has_value(); }
    bool smoking() const noexcept { return exercise_ == kSmoking; }
    bool keyboard_open() const noexcept { return focus_ != Focus::None; }

private:
    using Time = Clock::time_point;
    enum class Focus { None, Search, PlanName, Brand };

    // helpers (app.cpp)
    int u(float v) const;
    bool tapped(const Input& in, Rect r) const;
    bool tapped_in_view(const Input& in, Rect r) const;
    bool button(Canvas& c, const Input& in, Rect r, std::string_view label, Color base, int scale,
                bool in_content = true);
    void centered(Canvas& c, Rect area, int y, std::string_view s, int scale, Color col);
    void right_aligned(Canvas& c, int right, int y, std::string_view s, int scale, Color col);
    // Draws a card with four +/- buttons. Returns the delta of the tapped button, or 0.
    int stepper(Canvas& c, const Input& in, Rect card, std::string_view title, const std::string& value,
                const std::array<std::string_view, 4>& labels, const std::array<int, 4>& deltas);
    // Draws wrapping chips from the left margin and moves `y` below them. Returns the tapped index or -1.
    int chips(Canvas& c, const Input& in, int& y, std::span<const std::string_view> labels, int selected);
    // Draws a text box. Returns true if it was tapped.
    bool text_box(Canvas& c, const Input& in, Rect r, const std::string& text, std::string_view hint, bool focused);
    int empty_state(Canvas& c, int y, int icon, std::string_view title, std::string_view hint);
    // Draws the photo of an exercise, or an empty frame if it has none. `moving` flips between start and end.
    void draw_photo(Canvas& c, Rect r, int exercise, bool moving);

    void render(Canvas& c, const Input& in);
    int draw_workout(Canvas& c, const Input& in, int y);
    int draw_smoking(Canvas& c, const Input& in, int y);
    int draw_history(Canvas& c, int y);
    int draw_records(Canvas& c, int y);
    void draw_header(Canvas& c, const Input& in);
    void draw_tabs(Canvas& c, const Input& in);

    // picker.cpp
    int draw_picker(Canvas& c, const Input& in, int y);
    void draw_keyboard(Canvas& c, const Input& in);
    int keyboard_height() const;
    void type(char ch);
    std::string* focused_text();
    void choose_exercise(int index);

    // plan.cpp
    int draw_plan(Canvas& c, const Input& in, int y);
    int draw_plan_edit(Canvas& c, const Input& in, int y);
    const Plan* today_plan() const;
    void pick_plan_item(const PlanItem& item);

    void persist();

    Data data_;
    SaveFn save_;
    float unit_;

    Tab tab_ = Tab::Workout;
    Page page_ = Page::Main;
    int exercise_ = 0;
    int weight_x10_ = 200;
    int reps_ = 8;
    std::optional<std::size_t> active_;  // index of the running session
    std::optional<Time> rest_end_;
    std::chrono::seconds rest_len_{90};
    std::array<int, 6> scroll_{};        // one per Tab, then Picker, then PlanEdit

    // plans and search
    int today_ = 0;                      // Monday = 0
    int plan_day_ = 0;                   // day shown in the Plan tab
    int editing_ = -1;                   // plan open in the editor
    bool confirm_delete_ = false;
    bool pick_for_plan_ = false;
    int smoke_count_ = 1;
    std::string smoke_brand_;
    bool smoke_rolled_ = false;
    bool smoke_after_ = false;
    std::string search_;
    std::optional<Muscle> muscle_filter_;
    std::optional<Equipment> equipment_filter_;
    Focus focus_ = Focus::None;

    const ImagePack* images_ = nullptr;
    bool photo_animating_ = false;       // a moving photo is on screen: keep redrawing

    Time now_{};
    bool dirty_ = false;
    int inset_top_ = 0, inset_bottom_ = 0;
    int scroll_target_ = -1;             // scroll to this position at the start of the next frame
    int view_top_ = 0, view_bottom_ = 0;
    int s_small_ = 1, s_mid_ = 2, s_big_ = 4;
};

}  // namespace gym
