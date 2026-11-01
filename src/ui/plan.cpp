// Plan tab (week overview) and the plan editor.
#include <algorithm>
#include <format>

#include "ui/app.hpp"
#include "ui/style.hpp"

namespace gym {

using namespace style;

const Plan* App::today_plan() const {
    const int i = data_.week[static_cast<std::size_t>(today_)];
    return i >= 0 && i < static_cast<int>(data_.plans.size()) ? &data_.plans[static_cast<std::size_t>(i)] : nullptr;
}

int App::new_plan() {
    data_.plans.push_back({"NEW PLAN", {}});
    editing_ = static_cast<int>(data_.plans.size()) - 1;
    page_ = Page::PlanEdit;
    confirm_delete_ = false;
    persist();
    return editing_;
}

void App::edit_plan(int index) {
    if (index < 0 || index >= static_cast<int>(data_.plans.size())) return;
    editing_ = index;
    page_ = Page::PlanEdit;
    confirm_delete_ = false;
    dirty_ = true;
}

void App::delete_plan(int index) {
    if (index < 0 || index >= static_cast<int>(data_.plans.size())) return;
    data_.plans.erase(data_.plans.begin() + index);
    for (int& day : data_.week) {
        if (day == index) day = -1;
        else if (day > index) --day;
    }
    editing_ = -1;
    focus_ = Focus::None;
    page_ = Page::Main;
    tab_ = Tab::Plan;
    confirm_delete_ = false;
    persist();
}

void App::assign_day(int day, int plan) {
    if (day < 0 || day > 6 || plan < -1 || plan >= static_cast<int>(data_.plans.size())) return;
    data_.week[static_cast<std::size_t>(day)] = plan;
    persist();
}

void App::add_plan_item(int plan, int exercise) {
    if (plan < 0 || plan >= static_cast<int>(data_.plans.size())) return;
    if (exercise < 0 || exercise >= static_cast<int>(kExercises.size())) return;
    data_.plans[static_cast<std::size_t>(plan)].items.push_back({exercise, 3, 10});
    persist();
}

int App::draw_plan(Canvas& c, const Input& in, int y) {
    const int m = u(16), cw = c.width() - 2 * m;

    // The week: tap a day to see or change its plan.
    {
        const int gap = u(6), tw = (cw - 6 * gap) / 7, th = u(52);
        for (int d = 0; d < 7; ++d) {
            const Rect r{m + d * (tw + gap), y, tw, th};
            if (button(c, in, r, kWeekShort[static_cast<std::size_t>(d)], d == plan_day_ ? kAccent : kBtn, s_small_))
                plan_day_ = d;
            const int dot = u(6);
            if (data_.week[static_cast<std::size_t>(d)] >= 0)
                c.fill_round({r.x + (r.w - dot) / 2, r.bottom() - dot - u(6), dot, dot}, dot / 2,
                             d == plan_day_ ? kText : kGood);
            if (d == today_) c.fill_round({r.x + (r.w - u(16)) / 2, r.y + u(5), u(16), u(3)}, u(1), kText);
        }
        y += th + u(14);
    }

    // The chosen day
    const int day_plan = data_.week[static_cast<std::size_t>(plan_day_)];
    {
        const Rect card{m, y, cw, u(64)};
        c.fill_round(card, u(12), kCard);
        const bool is_today = plan_day_ == today_;
        c.text(card.x + u(14), card.y + u(12),
               std::string(kWeekLong[static_cast<std::size_t>(plan_day_)]) + (is_today ? " - TODAY" : ""), s_mid_, kText);
        const std::string label = day_plan >= 0 ? data_.plans[static_cast<std::size_t>(day_plan)].name : "REST DAY";
        c.text(card.x + u(14), card.y + u(12) + Canvas::text_height(s_mid_) + u(8), label, s_small_,
               day_plan >= 0 ? kGood : kDim);
        y = card.bottom() + u(14);
    }

    c.text(m, y, "PLAN FOR THIS DAY", s_small_, kDim);
    y += Canvas::text_height(s_small_) + u(8);
    {
        std::vector<std::string_view> labels{"REST"};
        for (const auto& p : data_.plans) labels.push_back(p.name);
        const int t = chips(c, in, y, labels, day_plan + 1);
        if (t >= 0) assign_day(plan_day_, t - 1);
    }
    y += u(6);

    if (day_plan >= 0) {
        const auto& plan = data_.plans[static_cast<std::size_t>(day_plan)];
        if (!plan.items.empty()) {
            const int row_h = u(34);
            const Rect card{m, y, cw, static_cast<int>(plan.items.size()) * row_h + u(16)};
            c.fill_round(card, u(12), kCard);
            int ry = card.y + u(8);
            for (const auto& item : plan.items) {
                const int ty = ry + (row_h - Canvas::text_height(s_mid_)) / 2;
                c.text(card.x + u(14), ty, kExercises[static_cast<std::size_t>(item.exercise)].name, s_mid_, kText);
                right_aligned(c, card.right() - u(14), ty, std::format("{} X {}", item.sets, item.reps), s_mid_, kDim);
                ry += row_h;
            }
            y = card.bottom() + u(14);
        }
    }

    // All plans
    c.text(m, y, "YOUR PLANS", s_small_, kDim);
    y += Canvas::text_height(s_small_) + u(8);
    if (data_.plans.empty()) {
        y = empty_state(c, y, static_cast<int>(Icon::Calendar), "NO PLANS YET", "TAP NEW PLAN TO MAKE ONE") + u(12);
    }
    for (std::size_t i = 0; i < data_.plans.size(); ++i) {
        const auto& plan = data_.plans[i];
        const Rect card{m, y, cw, u(72)};
        c.fill_round(card, u(12), kCard);
        const int block = Canvas::text_height(s_mid_) + u(8) + Canvas::text_height(s_small_);
        const int ty = card.y + (card.h - block) / 2;
        c.text(card.x + u(14), ty, plan.name, s_mid_, kText);
        c.text(card.x + u(14), ty + Canvas::text_height(s_mid_) + u(8), std::format("{} EXERCISES", plan.items.size()),
               s_small_, kDim);
        if (button(c, in, {card.right() - u(96), card.y + (card.h - u(40)) / 2, u(82), u(40)}, "EDIT", kBtn, s_small_))
            edit_plan(static_cast<int>(i));
        y = card.bottom() + u(10);
    }
    if (button(c, in, {m, y, cw, u(52)}, "+ NEW PLAN", kAccent, s_mid_)) new_plan();
    return y + u(52);
}

int App::draw_plan_edit(Canvas& c, const Input& in, int y) {
    if (editing_ < 0 || editing_ >= static_cast<int>(data_.plans.size())) {
        page_ = Page::Main;
        dirty_ = true;
        return y;
    }
    const int m = u(16), cw = c.width() - 2 * m;
    auto& plan = data_.plans[static_cast<std::size_t>(editing_)];

    const Rect box{m, y, cw, u(48)};
    if (text_box(c, in, box, plan.name, "PLAN NAME", focus_ == Focus::PlanName)) focus_ = Focus::PlanName;
    y += box.h + u(14);

    if (plan.items.empty()) {
        y = empty_state(c, y, static_cast<int>(Icon::Dumbbell), "NO EXERCISES", "ADD YOUR FIRST EXERCISE") + u(12);
    }

    // One card per exercise: sets and reps with - and + buttons.
    std::optional<std::size_t> remove;
    for (std::size_t i = 0; i < plan.items.size(); ++i) {
        auto& item = plan.items[i];
        const auto& ex = kExercises[static_cast<std::size_t>(item.exercise)];
        const Rect card{m, y, cw, u(112)};
        c.fill_round(card, u(12), kCard);
        c.text(card.x + u(14), card.y + u(12), ex.name, s_mid_, kText);
        c.text(card.x + u(14), card.y + u(12) + Canvas::text_height(s_mid_) + u(5),
               std::format("{} - {}", name(ex.muscle), name(ex.equipment)), s_small_, kDim);
        if (button(c, in, {card.right() - u(52), card.y + u(8), u(44), u(36)}, "X", kBtn, s_small_)) remove = i;

        const auto control = [&](int x, std::string_view label, int& value, int hi) {
            const int by = card.bottom() - u(52), bs = u(40);
            c.text(x, by + (bs - Canvas::text_height(s_small_)) / 2, label, s_small_, kDim);
            const int minus_x = x + Canvas::text_width(label, s_small_) + u(10);
            const Rect minus{minus_x, by, bs, bs}, plus{minus_x + bs + u(40), by, bs, bs};
            if (button(c, in, minus, "-", kBtn, s_mid_)) value = std::max(1, value - 1);
            if (button(c, in, plus, "+", kBtn, s_mid_)) value = std::min(hi, value + 1);
            centered(c, {minus.right(), by, plus.x - minus.right(), bs}, by + (bs - Canvas::text_height(s_mid_)) / 2,
                     std::format("{}", value), s_mid_, kText);
        };
        control(card.x + u(14), "SETS", item.sets, 20);
        control(card.x + card.w / 2 + u(14), "REPS", item.reps, 99);
        y = card.bottom() + u(10);
    }
    if (remove) {
        plan.items.erase(plan.items.begin() + static_cast<std::ptrdiff_t>(*remove));
        persist();
    }

    if (button(c, in, {m, y, cw, u(52)}, "+ ADD EXERCISE", kAccent, s_mid_)) {
        open_picker(true);
    }
    y += u(52) + u(24);

    const bool sure = confirm_delete_;
    if (button(c, in, {m, y, cw, u(48)}, sure ? "TAP AGAIN TO DELETE" : "DELETE PLAN", sure ? kBad : kBtn, s_small_)) {
        if (sure) delete_plan(editing_);
        else confirm_delete_ = true;
    }
    return y + u(48);
}

}  // namespace gym
