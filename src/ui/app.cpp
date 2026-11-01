#include "ui/app.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <format>

#include "core/stats.hpp"
#include "ui/style.hpp"

namespace gym {

using namespace style;

// ---- style helpers ----------------------------------------------------------

namespace style {

// Small vector-style icons made of rounded rects.
void draw_icon(Canvas& c, Icon icon, int x, int y, int s, Color col) {
    const int q = std::max(1, s / 6), t = std::max(1, s / 7);
    switch (icon) {
        case Icon::Dumbbell:
            c.fill({x, y + (s - t) / 2, s, t}, col);
            for (const int left : {x, x + s - q}) c.fill_round({left, y + s / 4, q, s / 2}, q / 3, col);
            for (const int left : {x + q, x + s - 2 * q}) c.fill_round({left, y, q, s}, q / 3, col);
            break;
        case Icon::List:
            for (int i = 0; i < 3; ++i) c.fill_round({x, y + i * (s - t) / 2, s, t}, t / 2, col);
            break;
        case Icon::Bars: {
            const int w = s / 4, heights[3] = {s / 2, s, s * 3 / 4};
            for (int i = 0; i < 3; ++i) c.fill_round({x + i * (s - w) / 2, y + s - heights[i], w, heights[i]}, w / 4, col);
            break;
        }
        case Icon::Cigarette: {
            // Paper in the given colour, orange filter on the left, red ember on the right.
            const int h = std::max(2, s / 4), by = y + (s - h) / 2;
            c.fill_round({x, by, s, h}, h / 3, col);
            c.fill_round({x, by, s / 4, h}, h / 3, kAccent);
            c.fill_round({x + s - s / 7, by, s / 7, h}, h / 3, kBad);
            break;
        }
        case Icon::Calendar: {
            const int b = std::max(1, s / 10), top = s / 4;
            c.fill_round({x, y + q / 2, s, s - q / 2}, q / 2, col);
            c.fill({x + b, y + top, s - 2 * b, s - top - b}, kCard);
            for (const int left : {x + s / 4, x + s - s / 4 - b * 2}) c.fill_round({left, y, b * 2, q}, b, col);
            for (int i = 0; i < 3; ++i) c.fill({x + s / 5 + i * (s / 4), y + s / 2, b * 2, b * 2}, col);
            break;
        }
    }
}

Color lighten(Color c) {
    const auto up = [](Color v) { return std::min<Color>(255, v + 28); };
    return rgb(static_cast<std::uint8_t>(up(c & 0xFF)), static_cast<std::uint8_t>(up((c >> 8) & 0xFF)),
               static_cast<std::uint8_t>(up((c >> 16) & 0xFF)));
}

}  // namespace style

namespace {

constexpr std::array<std::string_view, 7> kDays{"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
constexpr std::array<std::string_view, 12> kMonths{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                                   "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

std::string format_date(std::int64_t epoch) {
    const std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm{};
    localtime_r(&t, &tm);
    return std::format("{} {:02} {} {}  {:02}:{:02}", kDays[tm.tm_wday % 7], tm.tm_mday, kMonths[tm.tm_mon % 12],
                       tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
}

int weekday_index() {  // Monday = 0
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    return (tm.tm_wday + 6) % 7;
}

std::int64_t epoch_now() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// Long brand names must not run into the text on the right.
std::string shorten(const std::string& s, std::size_t max = 14) {
    return s.size() <= max ? s : s.substr(0, max - 1) + ".";
}

bool matches_word(const std::string& a, std::string_view b) {
    return std::ranges::equal(a, b, [](char x, char y) { return std::toupper(static_cast<unsigned char>(x)) == std::toupper(static_cast<unsigned char>(y)); });
}

std::string set_text(const SetEntry& e) {
    return std::format("{} KG X {}", kg(e.weight_x10), e.reps);
}

}  // namespace

App::App(Data data, SaveFn save, float unit) : data_(std::move(data)), save_(std::move(save)), unit_(unit) {
    // A workout started in the last 6 hours is still running.
    constexpr std::int64_t kResumeWindow = 6 * 60 * 60;
    if (!data_.log.empty() && epoch_now() - data_.log.back().start < kResumeWindow) active_ = data_.log.size() - 1;
    today_ = plan_day_ = weekday_index();
    select_exercise(0);
    // If there is a plan today, start with its first exercise.
    if (const Plan* p = today_plan(); p && !p->items.empty()) pick_plan_item(p->items.front());
}

int App::u(float v) const {
    return static_cast<int>(std::lround(v * unit_));
}

// ---- actions --------------------------------------------------------------

void App::select_exercise(int index) {
    const int n = static_cast<int>(kExercises.size());
    exercise_ = ((index % n) + n) % n;
    if (smoking()) {
        // Start from what you smoked last time.
        if (const auto last = last_smoke(data_.log)) {
            smoke_brand_ = last->brand;
            smoke_rolled_ = last->rolled;
        }
        dirty_ = true;
        return;
    }
    // Start from what you did last time.
    if (const auto last = last_set_for(data_.log, exercise_)) {
        weight_x10_ = last->weight_x10;
        reps_ = last->reps;
    } else {
        weight_x10_ = 200;
        reps_ = 8;
    }
    dirty_ = true;
}

void App::pick_plan_item(const PlanItem& item) {
    select_exercise(item.exercise);
    reps_ = item.reps;
}

void App::set_weight_x10(int v) {
    weight_x10_ = std::clamp(v, 0, 9999);
}

void App::set_reps(int v) {
    reps_ = std::clamp(v, 1, 99);
}

void App::log_set() {
    if (!active_) {
        data_.log.push_back({epoch_now(), {}, {}});
        active_ = data_.log.size() - 1;
    }
    data_.log[*active_].sets.push_back({exercise_, weight_x10_, reps_});
    rest_end_ = now_ + rest_len_;
    persist();
}

// Removes the last entry of the kind you are looking at: a smoke on the Smoking card, otherwise a set.
void App::undo_set() {
    if (!active_) return;
    auto& session = data_.log[*active_];
    if (smoking() && !session.smokes.empty()) session.smokes.pop_back();
    else if (!session.sets.empty()) session.sets.pop_back();
    else if (!session.smokes.empty()) session.smokes.pop_back();
    if (session.sets.empty() && session.smokes.empty()) {
        data_.log.erase(data_.log.begin() + static_cast<std::ptrdiff_t>(*active_));
        active_.reset();
    }
    persist();
}

void App::set_smoke(int count, std::string brand, bool rolled, bool after) {
    smoke_count_ = std::clamp(count, 1, 40);
    smoke_brand_ = std::move(brand);
    smoke_rolled_ = rolled;
    smoke_after_ = after;
    dirty_ = true;
}

void App::log_smoke() {
    if (!active_) {
        data_.log.push_back({epoch_now(), {}, {}});
        active_ = data_.log.size() - 1;
    }
    data_.log[*active_].smokes.push_back(
        {smoke_count_, smoke_brand_.empty() ? "UNKNOWN" : smoke_brand_, smoke_rolled_, smoke_after_});
    persist();
}

void App::finish_workout() {
    active_.reset();
    rest_end_.reset();
    dirty_ = true;
}

void App::set_tab(Tab t) noexcept {
    tab_ = t;
    page_ = Page::Main;
    focus_ = Focus::None;
    confirm_delete_ = false;
}

void App::set_insets(int top, int bottom) noexcept {
    if (top != inset_top_ || bottom != inset_bottom_) dirty_ = true;
    inset_top_ = std::max(0, top);
    inset_bottom_ = std::max(0, bottom);
}

bool App::back() {
    if (!can_go_back()) return false;
    dirty_ = true;
    if (focus_ != Focus::None) {
        focus_ = Focus::None;
        return true;
    }
    if (page_ == Page::Picker) {
        page_ = pick_for_plan_ ? Page::PlanEdit : Page::Main;
        return true;
    }
    if (page_ == Page::PlanEdit) {
        page_ = Page::Main;
        confirm_delete_ = false;
        return true;
    }
    return false;
}

void App::persist() {
    dirty_ = true;
    if (save_) save_(data_);
}

// ---- small drawing helpers --------------------------------------------------

bool App::tapped(const Input& in, Rect r) const {
    return in.released && !in.moved && r.contains(in.press_x, in.press_y) && r.contains(in.x, in.y);
}

// A tap on something that scrolls must not count if it started under the header or tab bar.
bool App::tapped_in_view(const Input& in, Rect r) const {
    return in.press_y >= view_top_ && in.press_y < view_bottom_ && tapped(in, r);
}

bool App::button(Canvas& c, const Input& in, Rect r, std::string_view label, Color base, int scale,
                 bool in_content) {
    const bool pressed = in.down && !in.moved && r.contains(in.press_x, in.press_y) && r.contains(in.x, in.y);
    c.fill_round(r, u(8), pressed ? lighten(base) : base);
    centered(c, r, r.y + (r.h - Canvas::text_height(scale)) / 2, label, scale, kText);
    if (in_content ? tapped_in_view(in, r) : tapped(in, r)) {
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
    c.fill_round({card.x + u(14), card.y + u(14), u(3), Canvas::text_height(s_small_)}, 1, kAccent);
    c.text(card.x + u(24), card.y + u(14), title, s_small_, kDim);
    centered(c, card, card.y + u(30), value, s_big_, kText);

    const int gap = u(8), bh = u(40);
    const int bw = (card.w - u(16) - 3 * gap) / 4;
    int result = 0;
    for (int i = 0; i < 4; ++i) {
        const Rect b{card.x + u(8) + i * (bw + gap), card.bottom() - bh - u(8), bw, bh};
        if (button(c, in, b, labels[static_cast<std::size_t>(i)], kBtn, s_mid_)) result = deltas[static_cast<std::size_t>(i)];
    }
    return result;
}

int App::chips(Canvas& c, const Input& in, int& y, std::span<const std::string_view> labels, int selected) {
    const int m = u(16), right = c.width() - m, h = u(34), gap = u(8), pad = u(12);
    int x = m, result = -1;
    for (std::size_t i = 0; i < labels.size(); ++i) {
        const int w = Canvas::text_width(labels[i], s_small_) + 2 * pad;
        if (x + w > right && x > m) {
            x = m;
            y += h + gap;
        }
        const bool on = static_cast<int>(i) == selected;
        if (button(c, in, {x, y, w, h}, labels[i], on ? kAccent : kBtn, s_small_)) result = static_cast<int>(i);
        x += w + gap;
    }
    y += h + gap;
    return result;
}

bool App::text_box(Canvas& c, const Input& in, Rect r, const std::string& text, std::string_view hint, bool focused) {
    if (focused) c.fill_round(r, u(10), kAccent);
    const int b = focused ? u(2) : 0;
    c.fill_round({r.x + b, r.y + b, r.w - 2 * b, r.h - 2 * b}, u(10) - b, focused ? kBg : kCard);
    const int ty = r.y + (r.h - Canvas::text_height(s_mid_)) / 2;
    if (text.empty() && !focused) c.text(r.x + u(14), ty, hint, s_mid_, kDim);
    else c.text(r.x + u(14), ty, focused ? text + "_" : text, s_mid_, kText);
    if (tapped_in_view(in, r)) {
        dirty_ = true;
        return true;
    }
    return false;
}

int App::empty_state(Canvas& c, int y, int icon, std::string_view title, std::string_view hint) {
    const int m = u(16), isz = u(36);
    const Rect card{m, y, c.width() - 2 * m, u(40) + isz + u(14) + Canvas::text_height(s_mid_) + u(8) +
                                                 Canvas::text_height(s_small_)};
    c.fill_round(card, u(12), kCard);
    draw_icon(c, static_cast<Icon>(icon), card.x + (card.w - isz) / 2, card.y + u(20), isz, kBtn);
    const int ty = card.y + u(20) + isz + u(14);
    centered(c, card, ty, title, s_mid_, kDim);
    centered(c, card, ty + Canvas::text_height(s_mid_) + u(8), hint, s_small_, kDim);
    return card.bottom();
}

void App::draw_photo(Canvas& c, Rect r, int exercise, bool moving) {
    using namespace std::chrono_literals;
    if (images_) {
        // Show the start position, then the end position, 0.7 s each.
        const int phase = moving ? static_cast<int>((now_.time_since_epoch() / 700ms) % ImagePack::kFrames) : 0;
        if (const auto img = images_->frame(exercise, phase)) {
            c.draw_image(r, *img);
            if (moving) photo_animating_ = true;
            return;
        }
    }
    c.fill_round(r, u(8), kLine);
    const int isz = r.h / 2;
    if (exercise == kSmoking) draw_icon(c, Icon::Cigarette, r.x + (r.w - isz) / 2, r.y + (r.h - isz) / 2, isz, kText);
    else draw_icon(c, Icon::Dumbbell, r.x + (r.w - isz) / 2, r.y + (r.h - isz) / 2, isz, kBtn);
}

// ---- screens ----------------------------------------------------------------

int App::draw_workout(Canvas& c, const Input& in, int y) {
    const int m = u(16), cw = c.width() - 2 * m;

    // Summary of the running workout
    {
        const bool on = active_.has_value();
        const auto sums = on ? summarize(data_.log[*active_]) : std::vector<ExerciseSummary>{};
        const std::array<std::pair<std::string_view, std::string>, 3> tiles{{
            {"SETS", on ? std::format("{}", data_.log[*active_].sets.size()) : "0"},
            {"VOLUME", std::format("{} KG", on ? volume_x10(data_.log[*active_]) / 10 : 0)},
            {"LIFTS", std::format("{}", sums.size())},
        }};
        const int gap = u(8), tw = (cw - 2 * gap) / 3, th = u(60);
        for (int i = 0; i < 3; ++i) {
            const Rect r{m + i * (tw + gap), y, tw, th};
            c.fill_round(r, u(12), kCard);
            const int block = Canvas::text_height(s_mid_) + u(6) + Canvas::text_height(s_small_);
            const int ty = r.y + (r.h - block) / 2;
            const auto& tile = tiles[static_cast<std::size_t>(i)];
            centered(c, r, ty, tile.second, s_mid_, on ? kText : kDim);
            centered(c, r, ty + Canvas::text_height(s_mid_) + u(6), tile.first, s_small_, kDim);
        }
        y += th + u(12);
    }

    // Today's plan: tap a line to pick that exercise.
    if (const Plan* plan = today_plan(); plan && !plan->items.empty()) {
        c.text(m, y, std::format("TODAY: {}", plan->name), s_small_, kDim);
        y += Canvas::text_height(s_small_) + u(8);
        const int row_h = u(40);
        const Rect card{m, y, cw, static_cast<int>(plan->items.size()) * row_h + u(12)};
        c.fill_round(card, u(12), kCard);
        int ry = card.y + u(6);
        for (const auto& item : plan->items) {
            const Rect row{card.x + u(6), ry, card.w - u(12), row_h};
            const int done = active_ ? static_cast<int>(std::ranges::count(data_.log[*active_].sets, item.exercise,
                                                                           &SetEntry::exercise))
                                     : 0;
            if (item.exercise == exercise_) c.fill_round(row, u(8), kLine);
            const int ty = row.y + (row.h - Canvas::text_height(s_mid_)) / 2;
            c.text(row.x + u(8), ty, kExercises[static_cast<std::size_t>(item.exercise)].name, s_mid_,
                   done >= item.sets ? kGood : kText);
            right_aligned(c, row.right() - u(8), ty, std::format("{}/{} X {}", done, item.sets, item.reps), s_mid_,
                          done >= item.sets ? kGood : kDim);
            if (tapped_in_view(in, row)) {
                pick_plan_item(item);
                dirty_ = true;
            }
            ry += row_h;
        }
        y = card.bottom() + u(12);
    }

    // Exercise card: photo in the middle, arrows on the sides. Tap the photo or the name to search.
    {
        const int bs = u(48);
        const int img_w = std::min(cw - 2 * bs - u(28), u(180)), img_h = img_w * 2 / 3;
        const int text_h = Canvas::text_height(s_mid_) + u(8) + Canvas::text_height(s_small_) + u(8) +
                           Canvas::text_height(s_small_);
        const Rect card{m, y, cw, u(14) + img_h + u(12) + text_h + u(14)};
        c.fill_round(card, u(12), kCard);
        const Rect photo{card.x + (card.w - img_w) / 2, card.y + u(14), img_w, img_h};
        const auto& ex = kExercises[static_cast<std::size_t>(exercise_)];
        draw_photo(c, photo, exercise_, true);

        const int by = photo.y + (photo.h - bs) / 2;
        if (button(c, in, {card.x + u(8), by, bs, bs}, "<", kBtn, s_big_ - 1)) select_exercise(exercise_ - 1);
        if (button(c, in, {card.right() - u(8) - bs, by, bs, bs}, ">", kBtn, s_big_ - 1))
            select_exercise(exercise_ + 1);

        int ty = photo.bottom() + u(12);
        centered(c, card, ty, ex.name, s_mid_, kText);
        ty += Canvas::text_height(s_mid_) + u(8);
        centered(c, card, ty, std::format("{} - {}", name(ex.muscle), name(ex.equipment)), s_small_, kDim);
        ty += Canvas::text_height(s_small_) + u(8);
        centered(c, card, ty, "TAP HERE TO SEARCH", s_small_, kAccent);
        if (tapped_in_view(in, {card.x + bs + u(16), card.y, card.w - 2 * (bs + u(16)), card.h})) open_picker(false);
        y = card.bottom() + u(12);
    }

    if (smoking()) {
        y = draw_smoking(c, in, y);
    } else {
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
    }

    // Rest timer: green outline and tinted fill so it stands out
    if (rest_end_ && now_ < *rest_end_) {
        const int th = Canvas::text_height(s_big_);
        const Rect card{m, y, cw, u(14) + Canvas::text_height(s_small_) + u(4) + th + u(12) + u(6) + u(14)};
        c.fill_round(card, u(12), kGood);
        c.fill_round({card.x + u(2), card.y + u(2), card.w - u(4), card.h - u(4)}, u(10), kGoodBg);
        const auto left = std::chrono::ceil<std::chrono::seconds>(*rest_end_ - now_).count();
        c.text(card.x + u(16), card.y + u(14), "REST", s_small_, kDim);
        c.text(card.x + u(16), card.y + u(18) + Canvas::text_height(s_small_),
               std::format("{:02}:{:02}", left / 60, left % 60), s_big_, kGood);
        const Rect bar{card.x + u(16), card.bottom() - u(20), card.w - u(32), u(6)};
        c.fill_round(bar, u(3), kBtn);
        const double frac = double(left) / double(rest_len_.count());
        c.fill_round({bar.x, bar.y, std::max(u(6), static_cast<int>(bar.w * std::clamp(frac, 0.0, 1.0))), bar.h}, u(3),
                     kGood);
        if (button(c, in, {card.right() - u(104), card.y + u(12), u(88), u(34)}, "SKIP", kBtn, s_small_))
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

    // Sets and smokes of the running workout, newest first
    c.text(m, y, "THIS WORKOUT", s_small_, kDim);
    y += Canvas::text_height(s_small_) + u(8);
    if (!active_) return empty_state(c, y, static_cast<int>(Icon::Dumbbell), "NO SETS YET", "LOG YOUR FIRST SET");
    const auto& session = data_.log[*active_];
    const int row_h = u(30);
    int end_y = y;
    if (!session.sets.empty()) {
        const auto& sets = session.sets;
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
        end_y = card.bottom();
        y = end_y + u(12);
    }
    if (!session.smokes.empty()) {
        c.text(m, y, "SMOKING", s_small_, kDim);
        y += Canvas::text_height(s_small_) + u(8);
        const auto& smokes = session.smokes;
        const Rect card{m, y, cw, static_cast<int>(smokes.size()) * row_h + u(16)};
        c.fill_round(card, u(12), kCard);
        int ry = card.y + u(8);
        for (std::size_t i = smokes.size(); i-- > 0;) {
            const auto& s = smokes[i];
            const int ty = ry + (row_h - Canvas::text_height(s_mid_)) / 2;
            c.text(card.x + u(14), ty, std::format("{} {} X {}", s.after ? "AFTER" : "BEFORE", s.count, shorten(s.brand)),
                   s_mid_, kBad);
            right_aligned(c, card.right() - u(14), ty, s.rolled ? "ROLLED" : "PACK", s_mid_, kDim);
            ry += row_h;
        }
        end_y = card.bottom();
    }
    return end_y;
}

// The Smoking exercise: how many, before or after training, from the pack or rolled, and the brand.
int App::draw_smoking(Canvas& c, const Input& in, int y) {
    const int m = u(16), cw = c.width() - 2 * m;
    constexpr std::array<std::string_view, 14> kBrands{"Marlboro", "Camel", "Lucky Strike", "Winston", "Chesterfield",
                                                      "L&M", "Pall Mall", "Parliament", "Newport", "Gauloises",
                                                      "Davidoff", "Drum", "Amber Leaf", "Golden Virginia"};
    constexpr std::array<std::string_view, 2> kWhen{"BEFORE TRAINING", "AFTER TRAINING"};
    constexpr std::array<std::string_view, 2> kType{"FROM THE PACK", "SELF ROLLED"};
    const auto label = [&](std::string_view text) {
        c.text(m, y, text, s_small_, kDim);
        y += Canvas::text_height(s_small_) + u(8);
    };

    {
        const Rect card{m, y, cw, u(124)};
        const int d = stepper(c, in, card, "CIGARETTES", std::format("{}", smoke_count_), {"-2", "-1", "+1", "+2"},
                              {-2, -1, 1, 2});
        if (d != 0) smoke_count_ = std::clamp(smoke_count_ + d, 1, 40);
        y = card.bottom() + u(14);
    }
    label("WHEN");
    if (const int t = chips(c, in, y, kWhen, smoke_after_ ? 1 : 0); t >= 0) smoke_after_ = t == 1;
    y += u(6);
    label("TYPE");
    if (const int t = chips(c, in, y, kType, smoke_rolled_ ? 1 : 0); t >= 0) smoke_rolled_ = t == 1;
    y += u(6);
    label("BRAND");
    {
        const Rect box{m, y, cw, u(48)};
        if (text_box(c, in, box, smoke_brand_, "TYPE A BRAND", focus_ == Focus::Brand)) {
            focus_ = Focus::Brand;
            // The keyboard will cover the lower half of the screen: put the box just under the header.
            scroll_target_ = scroll_[static_cast<std::size_t>(tab_)] + box.y - (view_top_ + u(16));
        }
        y += box.h + u(10);
    }
    {
        // Quick picks. The one that matches the box is lit.
        int selected = -1;
        for (std::size_t i = 0; i < kBrands.size(); ++i)
            if (matches_word(smoke_brand_, kBrands[i])) selected = static_cast<int>(i);
        if (const int t = chips(c, in, y, kBrands, selected); t >= 0) {
            smoke_brand_ = std::string(kBrands[static_cast<std::size_t>(t)]);
            focus_ = Focus::None;
        }
    }
    y += u(6);
    if (button(c, in, {m, y, cw, u(64)}, "LOG SMOKE", kBad, s_mid_)) log_smoke();
    y += u(64) + u(12);
    return y;
}

int App::draw_history(Canvas& c, int y) {
    const int m = u(16), cw = c.width() - 2 * m;
    if (data_.log.empty())
        return empty_state(c, y, static_cast<int>(Icon::List), "NO WORKOUTS YET", "FINISHED WORKOUTS SHOW UP HERE");
    for (auto s = data_.log.rbegin(); s != data_.log.rend(); ++s) {
        const auto sums = summarize(*s);
        const int head = u(12) + Canvas::text_height(s_mid_) + u(6) + Canvas::text_height(s_small_) + u(10);
        const int row_h = u(28);
        int before = 0, after = 0;
        for (const auto& smoke : s->smokes) (smoke.after ? after : before) += smoke.count;
        const int rows = static_cast<int>(sums.size()) + (s->smokes.empty() ? 0 : 1);
        const Rect card{m, y, cw, head + rows * row_h + u(8)};
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
        if (!s->smokes.empty()) {
            const int ty = ry + (row_h - Canvas::text_height(s_mid_)) / 2;
            c.text(card.x + u(14), ty, "SMOKING", s_mid_, kBad);
            std::string text;
            if (before > 0) text += std::format("{} BEFORE", before);
            if (after > 0) text += std::format("{}{} AFTER", text.empty() ? "" : "  ", after);
            right_aligned(c, card.right() - u(14), ty, text, s_mid_, kDim);
        }
        y = card.bottom() + u(12);
    }
    return y;
}

int App::draw_records(Canvas& c, int y) {
    const int m = u(16), cw = c.width() - 2 * m;
    const auto bests = personal_bests(data_.log);
    if (bests.empty())
        return empty_state(c, y, static_cast<int>(Icon::Bars), "NO RECORDS YET", "LOG SOME SETS TO SET A RECORD");
    c.text(m, y, "ESTIMATED 1 REP MAX", s_small_, kDim);
    y += Canvas::text_height(s_small_) + u(10);
    for (const auto& b : bests) {
        const Rect card{m, y, cw, u(72)};
        c.fill_round(card, u(12), kCard);
        const int block = Canvas::text_height(s_mid_) + u(8) + Canvas::text_height(s_small_);
        const int ty = card.y + (card.h - block) / 2;
        c.text(card.x + u(14), ty, kExercises[static_cast<std::size_t>(b.exercise)].name, s_mid_, kText);
        c.text(card.x + u(14), ty + Canvas::text_height(s_mid_) + u(8), "BEST " + set_text(b.top), s_small_, kDim);
        right_aligned(c, card.right() - u(14), card.y + (card.h - Canvas::text_height(s_mid_)) / 2,
                      kg(e1rm_x10(b.top)) + " KG", s_mid_, kAccent);
        y = card.bottom() + u(10);
    }
    return y;
}

void App::draw_header(Canvas& c, const Input& in) {
    const int hh = view_top_;
    const int bar = u(56);  // the header area below the status bar
    c.fill({0, 0, c.width(), hh}, kCard);
    c.fill({0, hh - 1, c.width(), 1}, kLine);
    const int top = inset_top_;
    if (page_ == Page::Main) {
        const int isz = u(26);
        draw_icon(c, Icon::Dumbbell, u(16), top + (bar - isz) / 2, isz, kAccent);
        c.text(u(16) + isz + u(10), top + (bar - Canvas::text_height(s_mid_)) / 2, "GYM LOG", s_mid_, kText);
        if (active_) {
            const std::string_view label = "ACTIVE";
            const int th = Canvas::text_height(s_small_), dot = u(8), pad = u(10);
            const int w = 2 * pad + dot + u(8) + Canvas::text_width(label, s_small_);
            const Rect pill{c.width() - u(16) - w, top + (bar - th - 2 * u(6)) / 2, w, th + 2 * u(6)};
            c.fill_round(pill, pill.h / 2, kGoodBg);
            c.fill_round({pill.x + pad, pill.y + (pill.h - dot) / 2, dot, dot}, dot / 2, kGood);
            c.text(pill.x + pad + dot + u(8), pill.y + u(6), label, s_small_, kGood);
        }
    } else {
        if (button(c, in, {u(8), top + u(8), u(40), u(40)}, "<", kBtn, s_mid_, false)) back();
        c.text(u(60), top + (bar - Canvas::text_height(s_mid_)) / 2,
               page_ == Page::Picker ? "CHOOSE EXERCISE" : "EDIT PLAN", s_mid_, kText);
    }
}

void App::draw_tabs(Canvas& c, const Input& in) {
    const int h = c.height() - view_bottom_;
    c.fill({0, view_bottom_, c.width(), h}, kCard);
    c.fill({0, view_bottom_, c.width(), 1}, kLine);
    constexpr std::array<std::string_view, 4> names{"WORKOUT", "PLAN", "HISTORY", "RECORDS"};
    constexpr std::array<Icon, 4> icons{Icon::Dumbbell, Icon::Calendar, Icon::List, Icon::Bars};
    const int w = c.width() / 4, isz = u(18), gap = u(6), bar = u(64);
    for (int i = 0; i < 4; ++i) {
        const auto k = static_cast<std::size_t>(i);
        const Rect r{i * w, view_bottom_, w, h};
        const bool selected = static_cast<int>(tab_) == i;
        const int top = r.y + (bar - isz - gap - Canvas::text_height(s_small_)) / 2;
        if (selected) c.fill_round({r.x + (r.w - u(40)) / 2, r.y, u(40), u(3)}, u(1), kAccent);
        draw_icon(c, icons[k], r.x + (r.w - isz) / 2, top, isz, selected ? kAccent : kDim);
        centered(c, r, top + isz + gap, names[k], s_small_, selected ? kText : kDim);
        if (tapped(in, r)) {
            set_tab(static_cast<Tab>(i));
            dirty_ = true;
        }
    }
}

// ---- frame ------------------------------------------------------------------

void App::render(Canvas& c, const Input& in) {
    photo_animating_ = false;
    s_small_ = std::max(1, u(1.2f));
    s_mid_ = std::max(2, u(1.6f));
    s_big_ = std::max(3, u(2.6f));

    const bool main = page_ == Page::Main;
    const int keyboard = focus_ != Focus::None ? keyboard_height() : 0;
    const bool tab_bar = main && keyboard == 0;  // the keyboard takes the place of the tab bar
    view_top_ = inset_top_ + u(56);
    view_bottom_ = tab_bar ? c.height() - inset_bottom_ - u(64) : c.height() - inset_bottom_ - keyboard;

    c.clear(kBg);

    // Content first. The header and tab bar are painted on top, so
    // scrolled content that goes under them is hidden.
    const int m = u(16);
    const auto id = static_cast<std::size_t>(main ? static_cast<int>(tab_) : (page_ == Page::Picker ? 4 : 5));
    auto& scroll = scroll_[id];
    if (scroll_target_ >= 0) {
        scroll = scroll_target_;
        scroll_target_ = -1;
    }
    const int y0 = view_top_ + m - scroll;
    int y_end = 0;
    if (page_ == Page::Picker) y_end = draw_picker(c, in, y0);
    else if (page_ == Page::PlanEdit) y_end = draw_plan_edit(c, in, y0);
    else switch (tab_) {
        case Tab::Workout: y_end = draw_workout(c, in, y0); break;
        case Tab::Plan: y_end = draw_plan(c, in, y0); break;
        case Tab::History: y_end = draw_history(c, y0); break;
        case Tab::Records: y_end = draw_records(c, y0); break;
    }
    const int content_h = y_end - y0 + 2 * m;
    const int max_scroll = std::max(0, content_h - (view_bottom_ - view_top_));
    if (const int clamped = std::clamp(scroll, 0, max_scroll); clamped != scroll) {
        scroll = clamped;
        dirty_ = true;
    }

    // Cover whatever content ran below the visible area.
    c.fill({0, view_bottom_, c.width(), c.height() - view_bottom_}, kBg);
    draw_header(c, in);
    if (tab_bar) draw_tabs(c, in);
    else if (keyboard > 0) draw_keyboard(c, in);
}

void App::frame(Canvas& canvas, const Input& in, Clock::time_point now) {
    now_ = now;
    today_ = weekday_index();
    if (rest_end_ && now_ >= *rest_end_) rest_end_.reset();
    if (in.down) {
        const auto id = static_cast<std::size_t>(page_ == Page::Main ? static_cast<int>(tab_)
                                                                     : (page_ == Page::Picker ? 4 : 5));
        scroll_[id] -= in.drag_dy;
    }

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
