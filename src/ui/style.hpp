#pragma once

#include <array>
#include <string_view>

#include "ui/canvas.hpp"

namespace gym::style {

inline constexpr Color kBg = rgb(18, 18, 24);
inline constexpr Color kCard = rgb(32, 33, 44);
inline constexpr Color kText = rgb(238, 238, 245);
inline constexpr Color kDim = rgb(150, 152, 170);
inline constexpr Color kAccent = rgb(255, 107, 53);
inline constexpr Color kGood = rgb(46, 204, 113);
inline constexpr Color kBad = rgb(214, 69, 65);
inline constexpr Color kBtn = rgb(52, 54, 72);
inline constexpr Color kLine = rgb(48, 50, 66);
inline constexpr Color kGoodBg = rgb(22, 52, 42);

inline constexpr std::array<std::string_view, 7> kWeekShort{"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
inline constexpr std::array<std::string_view, 7> kWeekLong{"MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY",
                                                           "FRIDAY", "SATURDAY", "SUNDAY"};

enum class Icon { Dumbbell, List, Bars, Calendar };

// Draws inside the square (x, y, s, s). The calendar hole uses the card colour.
void draw_icon(Canvas& c, Icon icon, int x, int y, int s, Color col);
Color lighten(Color c);

}  // namespace gym::style
