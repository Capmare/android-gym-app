#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace gym {

// Pixel layout: bytes R, G, B, A in memory. This matches Android RGBA_8888.
using Color = std::uint32_t;

constexpr Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
    return 0xFF000000u | (Color{b} << 16) | (Color{g} << 8) | Color{r};
}

struct Rect {
    int x{}, y{}, w{}, h{};
    constexpr int right() const noexcept { return x + w; }
    constexpr int bottom() const noexcept { return y + h; }
    constexpr bool contains(int px, int py) const noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

// 5x7 bitmap glyph. Each byte is one row, bit 4 is the left pixel.
std::array<std::uint8_t, 7> glyph(char c) noexcept;

class Canvas {
public:
    void resize(int w, int h);
    int width() const noexcept { return w_; }
    int height() const noexcept { return h_; }
    std::span<const std::uint32_t> row(int y) const { return {px_.data() + std::size_t(y) * w_, std::size_t(w_)}; }

    void clear(Color c);
    void fill(Rect r, Color c);
    void fill_round(Rect r, int radius, Color c);
    void text(int x, int y, std::string_view s, int scale, Color c);

    static constexpr int text_width(std::string_view s, int scale) noexcept {
        return s.empty() ? 0 : static_cast<int>(s.size()) * 6 * scale - scale;
    }
    static constexpr int text_height(int scale) noexcept { return 7 * scale; }

private:
    int w_{}, h_{};
    std::vector<std::uint32_t> px_;
};

}  // namespace gym
