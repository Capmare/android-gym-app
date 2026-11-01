// Exercise search page and the on-screen keyboard.
#include <algorithm>
#include <format>

#include "ui/app.hpp"
#include "ui/style.hpp"

namespace gym {

using namespace style;

void App::open_picker(bool for_plan) {
    page_ = Page::Picker;
    pick_for_plan_ = for_plan;
    search_.clear();
    muscle_filter_.reset();
    equipment_filter_.reset();
    focus_ = Focus::None;
    dirty_ = true;
}

void App::set_search(std::string text) {
    search_ = std::move(text);
    dirty_ = true;
}

void App::show_keyboard() {
    if (page_ == Page::Picker) focus_ = Focus::Search;
    else if (page_ == Page::PlanEdit) focus_ = Focus::PlanName;
    else focus_ = smoking() ? Focus::Brand : Focus::None;
    dirty_ = true;
}

void App::choose_exercise(int index) {
    focus_ = Focus::None;
    if (pick_for_plan_) {
        add_plan_item(editing_, index);
        page_ = Page::PlanEdit;
    } else {
        select_exercise(index);
        page_ = Page::Main;
        tab_ = Tab::Workout;
    }
    dirty_ = true;
}

std::string* App::focused_text() {
    if (focus_ == Focus::Search) return &search_;
    if (focus_ == Focus::Brand) return &smoke_brand_;
    if (focus_ == Focus::PlanName && editing_ >= 0 && editing_ < static_cast<int>(data_.plans.size()))
        return &data_.plans[static_cast<std::size_t>(editing_)].name;
    return nullptr;
}

void App::type(char ch) {
    std::string* text = focused_text();
    if (!text) return;
    if (ch == '\b') {
        if (!text->empty()) text->pop_back();
    } else if (text->size() < 24) {
        text->push_back(ch);
    }
    if (focus_ == Focus::PlanName) persist();
    dirty_ = true;
}

int App::draw_picker(Canvas& c, const Input& in, int y) {
    const int m = u(16), cw = c.width() - 2 * m;

    // Search box with a clear button
    const Rect box{m, y, cw, u(48)};
    if (text_box(c, in, box, search_, "SEARCH EXERCISES", focus_ == Focus::Search)) focus_ = Focus::Search;
    if (!search_.empty() &&
        button(c, in, {box.right() - u(44), box.y + u(6), u(36), u(36)}, "X", kBtn, s_small_)) {
        search_.clear();
    }
    y += box.h + u(14);

    // Muscle and equipment filters. Tap again to remove a filter.
    c.text(m, y, "MUSCLE", s_small_, kDim);
    y += Canvas::text_height(s_small_) + u(8);
    const int muscle = chips(c, in, y, kMuscleNames, muscle_filter_ ? static_cast<int>(*muscle_filter_) : -1);
    if (muscle >= 0) {
        const auto picked = static_cast<Muscle>(muscle);
        muscle_filter_ = muscle_filter_ == picked ? std::nullopt : std::optional{picked};
    }
    y += u(6);
    c.text(m, y, "EQUIPMENT", s_small_, kDim);
    y += Canvas::text_height(s_small_) + u(8);
    const int equipment = chips(c, in, y, kEquipmentNames, equipment_filter_ ? static_cast<int>(*equipment_filter_) : -1);
    if (equipment >= 0) {
        const auto picked = static_cast<Equipment>(equipment);
        equipment_filter_ = equipment_filter_ == picked ? std::nullopt : std::optional{picked};
    }
    y += u(6);

    // Results
    std::vector<int> found;
    for (std::size_t i = 0; i < kExercises.size(); ++i) {
        const auto& e = kExercises[i];
        if (muscle_filter_ && e.muscle != *muscle_filter_) continue;
        if (equipment_filter_ && e.equipment != *equipment_filter_) continue;
        if (!matches(e, search_)) continue;
        found.push_back(static_cast<int>(i));
    }
    c.text(m, y, std::format("{} EXERCISES", found.size()), s_small_, kDim);
    y += Canvas::text_height(s_small_) + u(8);
    if (found.empty()) return empty_state(c, y, static_cast<int>(Icon::List), "NO MATCHES", "TRY ANOTHER WORD OR FILTER");

    const int row_h = u(54), gap = u(6);
    const int block = Canvas::text_height(s_mid_) + u(5) + Canvas::text_height(s_small_);
    for (const int index : found) {
        const Rect row{m, y, cw, row_h};
        y += row_h + gap;
        if (row.bottom() < view_top_ || row.y > view_bottom_) continue;  // off screen: skip drawing
        const auto& e = kExercises[static_cast<std::size_t>(index)];
        const bool current = !pick_for_plan_ && index == exercise_;
        c.fill_round(row, u(10), current ? kLine : kCard);
        const int thumb_h = row.h - u(10), thumb_w = thumb_h * 3 / 2;
        draw_photo(c, {row.x + u(5), row.y + u(5), thumb_w, thumb_h}, index, false);
        const int tx = row.x + u(5) + thumb_w + u(12);
        const int ty = row.y + (row.h - block) / 2;
        c.text(tx, ty, fit(e.name, s_mid_, row.right() - u(10) - tx), s_mid_, kText);
        c.text(tx, ty + Canvas::text_height(s_mid_) + u(5),
               std::format("{} - {}", name(e.muscle), name(e.equipment)), s_small_, kDim);
        if (tapped_in_view(in, row)) {
            choose_exercise(index);
            dirty_ = true;
        }
    }
    return y;
}

int App::keyboard_height() const {
    return 5 * u(50) + u(10);
}

void App::draw_keyboard(Canvas& c, const Input& in) {
    const int kh = keyboard_height();
    const int top = c.height() - inset_bottom_ - kh;
    c.fill({0, top, c.width(), kh + inset_bottom_}, kCard);
    c.fill({0, top, c.width(), 1}, kLine);

    constexpr std::array<std::string_view, 4> rows{"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
    const int gap = u(4), key_h = u(44), key_w = (c.width() - 2 * u(6) - 9 * gap) / 10;
    const auto row_y = [&](int r) { return top + u(8) + r * u(50); };

    for (int r = 0; r < 4; ++r) {
        const auto keys = rows[static_cast<std::size_t>(r)];
        const int count = static_cast<int>(keys.size()) + (r == 3 ? 2 : 0);  // the delete key is 2 wide
        const int total = count * key_w + (static_cast<int>(keys.size()) + (r == 3 ? 1 : 0) - 1) * gap;
        int x = (c.width() - total) / 2;
        for (std::size_t i = 0; i < keys.size(); ++i) {
            if (button(c, in, {x, row_y(r), key_w, key_h}, keys.substr(i, 1), kBtn, s_mid_, false)) type(keys[i]);
            x += key_w + gap;
        }
        if (r == 3 && button(c, in, {x, row_y(r), 2 * key_w, key_h}, "DEL", kBtn, s_small_, false)) type('\b');
    }

    const int space_w = 6 * key_w + 5 * gap, done_w = 3 * key_w + 2 * gap;
    const int x = (c.width() - (space_w + gap + done_w)) / 2;
    if (button(c, in, {x, row_y(4), space_w, key_h}, "SPACE", kBtn, s_small_, false)) type(' ');
    if (button(c, in, {x + space_w + gap, row_y(4), done_w, key_h}, "DONE", kAccent, s_small_, false)) {
        focus_ = Focus::None;
        dirty_ = true;
    }
}

}  // namespace gym
