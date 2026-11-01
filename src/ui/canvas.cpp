#include "ui/canvas.hpp"

#include <algorithm>
#include <cmath>

namespace gym {

std::array<std::uint8_t, 7> glyph(char c) noexcept {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    switch (c) {
        case 'A': return {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        case 'B': return {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
        case 'C': return {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
        case 'D': return {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
        case 'E': return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
        case 'F': return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
        case 'G': return {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F};
        case 'H': return {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        case 'I': return {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
        case 'J': return {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C};
        case 'K': return {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
        case 'L': return {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
        case 'M': return {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
        case 'N': return {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11};
        case 'O': return {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        case 'P': return {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
        case 'Q': return {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
        case 'R': return {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
        case 'S': return {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
        case 'T': return {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
        case 'U': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        case 'V': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
        case 'W': return {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11};
        case 'X': return {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
        case 'Y': return {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
        case 'Z': return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
        case '0': return {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
        case '1': return {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
        case '2': return {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
        case '3': return {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E};
        case '4': return {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
        case '5': return {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E};
        case '6': return {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
        case '7': return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
        case '8': return {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
        case '9': return {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C};
        case '.': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C};
        case ',': return {0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08};
        case ':': return {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00};
        case '-': return {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
        case '+': return {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00};
        case '/': return {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10};
        case '(': return {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
        case ')': return {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};
        case '%': return {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03};
        case '!': return {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04};
        case '<': return {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02};
        case '>': return {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08};
        default: return {};  // space and unknown characters
    }
}

void Canvas::resize(int w, int h) {
    w_ = w;
    h_ = h;
    px_.assign(std::size_t(w) * h, 0);
}

void Canvas::clear(Color c) {
    std::ranges::fill(px_, c);
}

void Canvas::fill(Rect r, Color c) {
    const int x0 = std::max(r.x, 0), x1 = std::min(r.right(), w_);
    const int y0 = std::max(r.y, 0), y1 = std::min(r.bottom(), h_);
    if (x1 <= x0) return;
    for (int y = y0; y < y1; ++y) {
        auto first = px_.begin() + std::ptrdiff_t(y) * w_;
        std::fill(first + x0, first + x1, c);
    }
}

void Canvas::fill_round(Rect r, int radius, Color c) {
    radius = std::min({radius, r.w / 2, r.h / 2});
    for (int dy = 0; dy < r.h; ++dy) {
        int inset = 0;
        double yc = -1;  // distance from the corner circle centre, or negative if not in a corner row
        if (dy < radius) yc = radius - dy - 0.5;
        else if (dy >= r.h - radius) yc = dy - (r.h - radius) + 0.5;
        if (yc >= 0) inset = radius - static_cast<int>(std::sqrt(double(radius) * radius - yc * yc) + 0.5);
        fill({r.x + inset, r.y + dy, r.w - 2 * inset, 1}, c);
    }
}

void Canvas::text(int x, int y, std::string_view s, int scale, Color c) {
    for (const char ch : s) {
        const auto g = glyph(ch);
        for (int row = 0; row < 7; ++row)
            for (int bit = 0; bit < 5; ++bit)
                if (g[row] & (0x10 >> bit)) fill({x + bit * scale, y + row * scale, scale, scale}, c);
        x += 6 * scale;
    }
}

void Canvas::draw_image(Rect dst, const Image& img) {
    if (img.width <= 0 || img.height <= 0 || dst.w <= 0 || dst.h <= 0 ||
        img.rgb565.size() < static_cast<std::size_t>(img.width) * static_cast<std::size_t>(img.height) * 2)
        return;
    const auto* src = reinterpret_cast<const unsigned char*>(img.rgb565.data());
    const auto pixel = [&](int x, int y) {
        const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(img.width) + static_cast<std::size_t>(x)) * 2;
        return static_cast<unsigned>(src[i]) | (static_cast<unsigned>(src[i + 1]) << 8);
    };

    // For each output column or row: the two source pixels to mix, and how much of the second (0..255).
    struct Tap { int a, b, weight; };
    const auto make = [](int out_size, int in_size) {
        std::vector<Tap> taps(static_cast<std::size_t>(out_size));
        for (int i = 0; i < out_size; ++i) {
            const int pos = std::clamp(((2 * i + 1) * in_size * 128) / out_size - 128, 0, (in_size - 1) * 256);
            taps[static_cast<std::size_t>(i)] = {pos >> 8, std::min((pos >> 8) + 1, in_size - 1), pos & 255};
        }
        return taps;
    };
    const auto cols = make(dst.w, img.width), rows = make(dst.h, img.height);

    const int x0 = std::max(dst.x, 0), x1 = std::min(dst.right(), w_);
    const int y0 = std::max(dst.y, 0), y1 = std::min(dst.bottom(), h_);
    for (int y = y0; y < y1; ++y) {
        const Tap& ty = rows[static_cast<std::size_t>(y - dst.y)];
        auto* out = px_.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(w_);
        for (int x = x0; x < x1; ++x) {
            const Tap& tx = cols[static_cast<std::size_t>(x - dst.x)];
            const unsigned p00 = pixel(tx.a, ty.a), p10 = pixel(tx.b, ty.a), p01 = pixel(tx.a, ty.b), p11 = pixel(tx.b, ty.b);
            const auto mix = [&](unsigned shift, unsigned mask, unsigned scale_to_255) {
                const int a = static_cast<int>((p00 >> shift) & mask), b = static_cast<int>((p10 >> shift) & mask);
                const int c = static_cast<int>((p01 >> shift) & mask), d = static_cast<int>((p11 >> shift) & mask);
                const int top = a * (256 - tx.weight) + b * tx.weight, bottom = c * (256 - tx.weight) + d * tx.weight;
                const int v = (top * (256 - ty.weight) + bottom * ty.weight) >> 16;  // 0..mask
                return static_cast<std::uint8_t>(v * static_cast<int>(scale_to_255) / static_cast<int>(mask));
            };
            out[x] = rgb(mix(11, 31, 255), mix(5, 63, 255), mix(0, 31, 255));
        }
    }
}

}  // namespace gym
