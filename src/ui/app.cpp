#include "ui/app.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <format>

#include "core/stats.hpp"

namespace gym {

namespace {

constexpr Color kBg = rgb(18, 18, 24);
constexpr Color kCard = rgb(32, 33, 44);
constexpr Color kText = rgb(238, 238, 245);
constexpr Color kDim = rgb(150, 152, 170);
constexpr Color kAccent = rgb(255, 107, 53);
constexpr Color kGood = rgb(46, 204, 113);
constexpr Color kBtn = rgb(52, 54, 72);

constexpr std::array<std::string_view, 7> kDays{"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
constexpr std::array<std::string_view, 12> kMonths{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                                   "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

Color lighten(Color c) {
    const auto up = [](Color v) { return std::min<Color>(255, v + 28); };
    return rgb(static_cast<std::uint8_t>(up(c & 0xFF)), static_cast<std::uint8_t>(up((c >> 8) & 0xFF)),
               static_cast<std::uint8_t>(up((c >> 16) & 0xFF)));
}

std::string format_date(std::int64_t epoch) {
    const std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm{};
    localtime_r(&t, &tm);
    return std::format("{} {:02} {} {}  {:02}:{:02}", kDays[tm.tm_wday % 7], tm.tm_mday, kMonths[tm.tm_mon % 12],
                       tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
}

std::int64_t epoch_now() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string set_text(const SetEntry& e) {
    return std::format("{} KG X {}", kg(e.weight_x10), e.reps);
}

}  // namespace

App::App(Log log, SaveFn save, float unit) : log_(std::move(log)), save_(std::move(save)), unit_(unit) {
    // A workout started in the last 6 hours is still running.
    constexpr std::int64_t kResumeWindow = 6 * 60 * 60;
    if (!log_.empty() && epoch_now() - log_.back().start < kResumeWindow) active_ = log_.size() - 1;
    select_exercise(0);
}

int App::u(float v) const {
    return static_cast<int>(std::lround(v * unit_));
}

// ---- actions --------------------------------------------------------------

void App::select_exercise(int index) {
    const int n = static_cast<int>(kExercises.size());
    exercise_ = ((index % n) + n) % n;
    // Start from what you did last time.
    if (const auto last = last_set_for(log_, exercise_)) {
        weight_x10_ = last->weight_x10;
        reps_ = last->reps;
    } else {
        weight_x10_ = 200;
        reps_ = 8;
    }
    dirty_ = true;
}

void App::set_weight_x10(int v) {
    weight_x10_ = std::clamp(v, 0, 9999);
}

void App::set_reps(int v) {
    reps_ = std::clamp(v, 1, 99);
}

void App::log_set() {
    if (!active_) {
        log_.push_back({epoch_now(), {}});
        active_ = log_.size() - 1;
    }
    log_[*active_].sets.push_back({exercise_, weight_x10_, reps_});
    rest_end_ = now_ + rest_len_;
    persist();
}

void App::undo_set() {
    if (!active_) return;
    auto& sets = log_[*active_].sets;
    if (!sets.empty()) sets.pop_back();
    if (sets.empty()) {
        log_.erase(log_.begin() + static_cast<std::ptrdiff_t>(*active_));
        active_.reset();
    }
    persist();
}

void App::finish_workout() {
    active_.reset();
    rest_end_.reset();
    dirty_ = true;
}

void App::persist() {
    dirty_ = true;
    if (save_) save_(log_);
}

// ---- small drawing helpers --------------------------------------------------

bool App::tapped(const Input& in, Rect r) const {
    return in.released && !in.moved && r.contains(in.press_x, in.press_y) && r.contains(in.x, in.y);
}

bool App::button(Canvas& c, const Input& in, Rect r, std::string_view label, Color base, int scale) {
    const bool pressed = in.down && !in.moved && r.contains(in.press_x, in.press_y) && r.contains(in.x, in.y);
    c.fill_round(r, u(8), pressed ? lighten(base) : base);
    centered(c, r, r.y + (r.h - Canvas::text_height(scale)) / 2, label, scale, kText);
    // A button hidden under the header or tab bar must not react.
    const bool visible = in.press_y >= view_top_ && in.press_y < view_bottom_;
    if (visible && tapped(in, r)) {
        dirty_ = true;
        return true;
    }
    return false;
}

void App::centered(Canvas& c, Rect area, int y, std::string_view s, int scale, Color col) {
    c.text(area.x + (area.w - Canvas::text_width(s, scale)) / 2, y, s, scale, col);
}

void App::right_aligned(Canvas& c, int right, int y, std::string_view s, int scale, Color col) {
    c.text(right - Canvas::text_width(s, scale), y, s, scale, col);
}

int App::stepper(Canvas& c, const Input& in, Rect card, std::string_view title, const std::string& value,
                 const std::array<std::string_view, 4>& labels, const std::array<int, 4>& deltas) {
    c.fill_round(card, u(12), kCard);
    c.text(card.x + u(14), card.y + u(12), title, s_small_, kDim);
    centered(c, card, card.y + u(30), value, s_big_, kText);

    const int gap = u(8), bh = u(40);
    const int bw = (card.w - u(16) - 3 * gap) / 4;
    int result = 0;
    for (int i = 0; i < 4; ++i) {
        const Rect b{card.x + u(8) + i * (bw + gap), card.bottom() - bh - u(8), bw, bh};
        if (button(c, in, b, labels[i], kBtn, s_mid_)) result = deltas[i];
    }
    return result;
}

// ---- screens ----------------------------------------------------------------

int App::draw_workout(Canvas& c, const Input& in, int y) {
    const int m = u(16), cw = c.width() - 2 * m;

    // Exercise picker
    {
        const Rect card{m, y, cw, u(84)};
        c.fill_round(card, u(12), kCard);
        const int bs = u(48);
        const int by = card.y + (card.h - bs) / 2;
        if (button(c, in, {card.x + u(8), by, bs, bs}, "<", kBtn, s_big_ - 1)) select_exercise(exercise_ - 1);
        if (button(c, in, {card.right() - u(8) - bs, by, bs, bs}, ">", kBtn, s_big_ - 1))
            select_exercise(exercise_ + 1);
        const Rect mid{card.x + bs + u(16), card.y, card.w - 2 * bs - u(32), card.h};
        const auto& ex = kExercises[static_cast<std::size_t>(exercise_)];
        centered(c, mid, card.y + u(20), ex.name, s_mid_, kText);
        centered(c, mid, card.y + u(20) + Canvas::text_height(s_mid_) + u(8), ex.muscle, s_small_, kDim);
        y = card.bottom() + u(12);
    }

    // Weight and reps
    {
        const Rect card{m, y, cw, u(124)};
        const int d = stepper(c, in, card, "WEIGHT", kg(weight_x10_) + " KG", {"-5", "-2.5", "+2.5", "+5"},
                              {-50, -25, 25, 50});
        if (d != 0) set_weight_x10(weight_x10_ + d);
        y = card.bottom() + u(12);
    }
    {
        const Rect card{m, y, cw, u(124)};
        const int d = stepper(c, in, card, "REPS", std::format("{}", reps_), {"-2", "-1", "+1", "+2"}, {-2, -1, 1, 2});
        if (d != 0) set_reps(reps_ + d);
        y = card.bottom() + u(12);
    }

    // Log button
    if (button(c, in, {m, y, cw, u(64)}, "LOG SET", kAccent, s_mid_)) log_set();
    y += u(64) + u(12);

    // Rest timer
    if (rest_end_ && now_ < *rest_end_) {
        const Rect card{m, y, cw, u(60)};
        c.fill_round(card, u(12), kCard);
        const auto left = std::chrono::ceil<std::chrono::seconds>(*rest_end_ - now_).count();
        c.text(card.x + u(14), card.y + u(12), std::format("REST {:02}:{:02}", left / 60, left % 60), s_mid_, kGood);
        const Rect bar{card.x + u(14), card.bottom() - u(14), card.w - u(28), u(4)};
        c.fill(bar, kBtn);
        const double frac = double(left) / double(rest_len_.count());
        c.fill({bar.x, bar.y, static_cast<int>(bar.w * std::clamp(frac, 0.0, 1.0)), bar.h}, kGood);
        if (button(c, in, {card.right() - u(88), card.y + u(10), u(76), u(32)}, "SKIP", kBtn, s_small_))
            rest_end_.reset();
        y = card.bottom() + u(12);
    }

    // Undo and finish
    {
        const int half = (cw - u(12)) / 2;
        if (button(c, in, {m, y, half, u(48)}, "UNDO", kBtn, s_small_)) undo_set();
        if (button(c, in, {m + half + u(12), y, half, u(48)}, "FINISH", active_ ? kGood : kBtn, s_small_))
            finish_workout();
        y += u(48) + u(16);
    }

    // Sets of the running workout, newest first
    c.text(m, y, "THIS WORKOUT", s_small_, kDim);
    y += Canvas::text_height(s_small_) + u(8);
    if (!active_) {
        c.text(m, y, "NO SETS YET. LOG YOUR FIRST SET!", s_small_, kDim);
        return y + Canvas::text_height(s_small_);
    }
    const auto& sets = log_[*active_].sets;
    const int row_h = u(30);
    const Rect card{m, y, cw, static_cast<int>(sets.size()) * row_h + u(16)};
    c.fill_round(card, u(12), kCard);
    int ry = card.y + u(8);
    for (std::size_t i = sets.size(); i-- > 0;) {
        const auto& e = sets[i];
        const int ty = ry + (row_h - Canvas::text_height(s_mid_)) / 2;
        c.text(card.x + u(14), ty, std::format("{}. {}", i + 1, kExercises[static_cast<std::size_t>(e.exercise)].name),
               s_mid_, kText);
        right_aligned(c, card.right() - u(14), ty, set_text(e), s_mid_, kDim);
        ry += row_h;
    }
    return card.bottom();
}

int App::draw_history(Canvas& c, int y) {
    const int m = u(16), cw = c.width() - 2 * m;
    if (log_.empty()) {
        centered(c, {0, 0, c.width(), 0}, y + u(40), "NO WORKOUTS YET", s_mid_, kDim);
        return y + u(80);
    }
    for (auto s = log_.rbegin(); s != log_.rend(); ++s) {
        const auto sums = summarize(*s);
        const int head = u(12) + Canvas::text_height(s_mid_) + u(6) + Canvas::text_height(s_small_) + u(10);
        const int row_h = u(28);
        const Rect card{m, y, cw, head + static_cast<int>(sums.size()) * row_h + u(8)};
        c.fill_round(card, u(12), kCard);
        c.text(card.x + u(14), card.y + u(12), format_date(s->start), s_mid_, kText);
        c.text(card.x + u(14), card.y + u(12) + Canvas::text_height(s_mid_) + u(6),
               std::format("{} SETS   VOLUME {} KG", s->sets.size(), volume_x10(*s) / 10), s_small_, kDim);
        int ry = card.y + head;
        for (const auto& e : sums) {
            const int ty = ry + (row_h - Canvas::text_height(s_mid_)) / 2;
            c.text(card.x + u(14), ty, kExercises[static_cast<std::size_t>(e.exercise)].name, s_mid_, kAccent);
            right_aligned(c, card.right() - u(14), ty, std::format("{} X {}", e.sets, set_text(e.top)), s_mid_, kDim);
            ry += row_h;
        }
        y = card.bottom() + u(12);
    }
    return y;
}

int App::draw_records(Canvas& c, int y) {
    const int m = u(16), cw = c.width() - 2 * m;
    const auto bests = personal_bests(log_);
    if (bests.empty()) {
        centered(c, {0, 0, c.width(), 0}, y + u(40), "NO RECORDS YET", s_mid_, kDim);
        return y + u(80);
    }
    c.text(m, y, "ESTIMATED 1 REP MAX", s_small_, kDim);
    y += Canvas::text_height(s_small_) + u(10);
    for (const auto& b : bests) {
        const Rect card{m, y, cw, u(72)};
        c.fill_round(card, u(12), kCard);
        c.text(card.x + u(14), card.y + u(12), kExercises[static_cast<std::size_t>(b.exercise)].name, s_mid_, kText);
        c.text(card.x + u(14), card.y + u(12) + Canvas::text_height(s_mid_) + u(8), "BEST " + set_text(b.top),
               s_small_, kDim);
        right_aligned(c, card.right() - u(14), card.y + (card.h - Canvas::text_height(s_mid_)) / 2,
                      kg(e1rm_x10(b.top)) + " KG", s_mid_, kAccent);
        y = card.bottom() + u(10);
    }
    return y;
}

void App::draw_header(Canvas& c) {
    const int hh = view_top_;
    c.fill({0, 0, c.width(), hh}, kCard);
    const int ty = (hh - Canvas::text_height(s_mid_)) / 2;
    c.text(u(16), ty, "GYM LOG", s_mid_, kAccent);
    if (active_)
        right_aligned(c, c.width() - u(16), (hh - Canvas::text_height(s_small_)) / 2, "WORKOUT ACTIVE", s_small_, kGood);
}

void App::draw_tabs(Canvas& c, const Input& in) {
    const int h = c.height() - view_bottom_;
    c.fill({0, view_bottom_, c.width(), h}, kCard);
    constexpr std::array<std::string_view, 3> names{"WORKOUT", "HISTORY", "RECORDS"};
    const int w = c.width() / 3;
    for (int i = 0; i < 3; ++i) {
        const Rect r{i * w, view_bottom_, w, h};
        const bool selected = static_cast<int>(tab_) == i;
        if (selected) c.fill({r.x, r.y, r.w, u(3)}, kAccent);
        centered(c, r, r.y + (r.h - Canvas::text_height(s_small_)) / 2, names[static_cast<std::size_t>(i)], s_small_,
                 selected ? kText : kDim);
        if (tapped(in, r)) {
            tab_ = static_cast<Tab>(i);
            dirty_ = true;
        }
    }
}

// ---- frame ------------------------------------------------------------------

void App::render(Canvas& c, const Input& in) {
    s_small_ = std::max(1, u(1.2f));
    s_mid_ = std::max(2, u(1.6f));
    s_big_ = std::max(3, u(2.6f));
    view_top_ = u(56);
    view_bottom_ = c.height() - u(60);

    c.clear(kBg);

    // Content first. The header and tab bar are painted on top, so
    // scrolled content that goes under them is hidden.
    const int m = u(16);
    auto& scroll = scroll_[static_cast<std::size_t>(tab_)];
    const int y0 = view_top_ + m - scroll;
    int y_end = 0;
    switch (tab_) {
        case Tab::Workout: y_end = draw_workout(c, in, y0); break;
        case Tab::History: y_end = draw_history(c, y0); break;
        case Tab::Records: y_end = draw_records(c, y0); break;
    }
    const int content_h = y_end - y0 + 2 * m;
    const int max_scroll = std::max(0, content_h - (view_bottom_ - view_top_));
    if (const int clamped = std::clamp(scroll, 0, max_scroll); clamped != scroll) {
        scroll = clamped;
        dirty_ = true;
    }

    draw_header(c);
    draw_tabs(c, in);
}

void App::frame(Canvas& canvas, const Input& in, Clock::time_point now) {
    now_ = now;
    if (rest_end_ && now_ >= *rest_end_) rest_end_.reset();
    if (in.down) scroll_[static_cast<std::size_t>(tab_)] -= in.drag_dy;

    dirty_ = false;
    render(canvas, in);
    // A tap changes the state while the screen is being drawn.
    // Draw again so the user sees the new state at once.
    if (dirty_) {
        dirty_ = false;
        render(canvas, Input{});
    }
}

}  // namespace gym
