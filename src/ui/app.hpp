#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

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

enum class Tab { Workout, History, Records };

class App {
public:
    using Clock = std::chrono::steady_clock;
    using SaveFn = std::function<void(const Log&)>;

    // `unit` is the screen density factor: 1.0 = 160 dpi, 2.0 = 320 dpi.
    App(Log log, SaveFn save, float unit);

    // Draw one frame. May change the state if the user taps a button.
    void frame(Canvas& canvas, const Input& in, Clock::time_point now);

    // True while the screen must repaint without any touch (rest timer).
    bool animating() const noexcept { return rest_end_.has_value(); }

    // Actions. The buttons call these. Tests can call them too.
    void select_exercise(int index);  // wraps around
    void log_set();
    void undo_set();
    void finish_workout();
    void set_weight_x10(int v);
    void set_reps(int v);
    void set_tab(Tab t) noexcept { tab_ = t; }

    const Log& log() const noexcept { return log_; }
    Tab tab() const noexcept { return tab_; }
    int exercise() const noexcept { return exercise_; }
    int weight_x10() const noexcept { return weight_x10_; }
    int reps() const noexcept { return reps_; }
    bool workout_active() const noexcept { return active_.has_value(); }

private:
    using Time = Clock::time_point;

    int u(float v) const;
    bool tapped(const Input& in, Rect r) const;
    bool button(Canvas& c, const Input& in, Rect r, std::string_view label, Color base, int scale);
    void centered(Canvas& c, Rect area, int y, std::string_view s, int scale, Color col);
    void right_aligned(Canvas& c, int right, int y, std::string_view s, int scale, Color col);
    // Draws a card with four +/- buttons. Returns the delta of the tapped button, or 0.
    int stepper(Canvas& c, const Input& in, Rect card, std::string_view title, const std::string& value,
                const std::array<std::string_view, 4>& labels, const std::array<int, 4>& deltas);

    void render(Canvas& c, const Input& in);
    int draw_workout(Canvas& c, const Input& in, int y);
    int draw_history(Canvas& c, int y);
    int draw_records(Canvas& c, int y);
    int empty_state(Canvas& c, int y, int icon, std::string_view title, std::string_view hint);
    void draw_header(Canvas& c);
    void draw_tabs(Canvas& c, const Input& in);

    void persist();

    Log log_;
    SaveFn save_;
    float unit_;

    Tab tab_ = Tab::Workout;
    int exercise_ = 0;
    int weight_x10_ = 200;
    int reps_ = 8;
    std::optional<std::size_t> active_;  // index of the running session
    std::optional<Time> rest_end_;
    std::chrono::seconds rest_len_{90};
    std::array<int, 3> scroll_{};

    Time now_{};
    bool dirty_ = false;
    int view_top_ = 0, view_bottom_ = 0;
    int s_small_ = 1, s_mid_ = 2, s_big_ = 4;
};

}  // namespace gym
