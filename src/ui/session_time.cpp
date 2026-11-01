// Changing when a workout happened, and the page for it.
#include <algorithm>
#include <chrono>
#include <ctime>
#include <format>
#include <numeric>

#include "ui/app.hpp"
#include "ui/clock.hpp"
#include "ui/style.hpp"

namespace gym {

using namespace style;

namespace {

constexpr std::array<std::string_view, 7> kDays{"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
constexpr std::array<std::string_view, 12> kMonths{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                                   "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

std::tm local_time(std::int64_t epoch) {
    const std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm{};
    localtime_r(&t, &tm);
    return tm;
}

}  // namespace

std::int64_t epoch_now() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string format_date(std::int64_t epoch) {
    const std::tm tm = local_time(epoch);
    return std::format("{} {:02} {} {}  {:02}:{:02}", kDays[static_cast<std::size_t>(tm.tm_wday % 7)], tm.tm_mday,
                       kMonths[static_cast<std::size_t>(tm.tm_mon % 12)], tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
}

void App::edit_time(int session) {
    if (session < 0 || session >= static_cast<int>(data_.log.size())) return;
    editing_session_ = session;
    page_ = Page::TimeEdit;
    focus_ = Focus::None;
    dirty_ = true;
}

// A workout cannot start in the future. After the change the workouts are sorted by time again,
// so the list may reorder. `editing_session_` and the running workout follow their workout.
void App::set_session_start(int session, std::int64_t start) {
    if (session < 0 || session >= static_cast<int>(data_.log.size())) return;
    auto& log = data_.log;
    log[static_cast<std::size_t>(session)].start = std::min(start, epoch_now());

    std::vector<std::size_t> order(log.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::ranges::stable_sort(order, {}, [&](std::size_t i) { return log[i].start; });

    const auto old_active = active_;
    Log sorted;
    sorted.reserve(log.size());
    for (std::size_t pos = 0; pos < order.size(); ++pos) {
        if (order[pos] == static_cast<std::size_t>(session)) editing_session_ = static_cast<int>(pos);
        if (old_active && order[pos] == *old_active) active_ = pos;
        sorted.push_back(std::move(log[order[pos]]));
    }
    log = std::move(sorted);
    persist();
}

void App::shift_session_minutes(int session, int minutes) {
    if (session < 0 || session >= static_cast<int>(data_.log.size())) return;
    set_session_start(session, data_.log[static_cast<std::size_t>(session)].start + 60LL * minutes);
}

void App::shift_session_days(int session, int days) {
    if (session < 0 || session >= static_cast<int>(data_.log.size())) return;
    std::tm tm = local_time(data_.log[static_cast<std::size_t>(session)].start);
    tm.tm_mday += days;
    tm.tm_isdst = -1;  // let the system decide about daylight saving
    set_session_start(session, static_cast<std::int64_t>(std::mktime(&tm)));
}

int App::draw_time_edit(Canvas& c, const Input& in, int y) {
    if (editing_session_ < 0 || editing_session_ >= static_cast<int>(data_.log.size())) {
        page_ = Page::Main;
        dirty_ = true;
        return y;
    }
    const int m = u(16), cw = c.width() - 2 * m;
    const int index = editing_session_;
    const auto& session = data_.log[static_cast<std::size_t>(index)];
    const std::tm tm = local_time(session.start);

    // What is being edited
    {
        const std::string text = format_date(session.start);
        int scale = s_big_;
        while (scale > 1 && Canvas::text_width(text, scale) > cw - u(28)) --scale;
        const Rect card{m, y, cw, u(24) + Canvas::text_height(s_small_) + u(10) + Canvas::text_height(scale) + u(12) +
                                      Canvas::text_height(s_small_) + u(14)};
        c.fill_round(card, u(12), kCard);
        c.text(card.x + u(14), card.y + u(14), "WORKOUT STARTED", s_small_, kDim);
        const int ty = card.y + u(14) + Canvas::text_height(s_small_) + u(10);
        c.text(card.x + u(14), ty, text, scale, kText);
        c.text(card.x + u(14), ty + Canvas::text_height(scale) + u(12),
               std::format("{} SETS, {} SMOKES", session.sets.size(), session.smokes.size()), s_small_, kDim);
        y = card.bottom() + u(12);
    }

    const auto step = [&](std::string_view title, const std::string& value, const std::array<std::string_view, 4>& labels,
                          const std::array<int, 4>& deltas) {
        const Rect card{m, y, cw, u(124)};
        y = card.bottom() + u(12);
        return stepper(c, in, card, title, value, labels, deltas);
    };
    if (const int d = step("DATE", std::format("{} {:02} {}", kDays[static_cast<std::size_t>(tm.tm_wday % 7)], tm.tm_mday,
                                               kMonths[static_cast<std::size_t>(tm.tm_mon % 12)]),
                           {"-7D", "-1D", "+1D", "+7D"}, {-7, -1, 1, 7});
        d != 0)
        shift_session_days(index, d);
    if (const int d = step("HOUR", std::format("{:02}", tm.tm_hour), {"-3H", "-1H", "+1H", "+3H"}, {-180, -60, 60, 180});
        d != 0)
        shift_session_minutes(index, d);
    if (const int d = step("MINUTE", std::format("{:02}", tm.tm_min), {"-15", "-5", "+5", "+15"}, {-15, -5, 5, 15});
        d != 0)
        shift_session_minutes(index, d);

    if (button(c, in, {m, y, cw, u(56)}, "DONE", kAccent, s_mid_)) back();
    return y + u(56);
}

}  // namespace gym
