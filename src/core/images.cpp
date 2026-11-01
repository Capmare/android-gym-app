#include "core/images.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace gym {

namespace {

constexpr std::size_t kHeader = 16;

std::uint32_t read_le(std::span<const std::byte> data, std::size_t at, int bytes) {
    std::uint32_t v = 0;
    for (int i = 0; i < bytes; ++i) v |= static_cast<std::uint32_t>(data[at + static_cast<std::size_t>(i)]) << (8 * i);
    return v;
}

}  // namespace

ImagePack::ImagePack(std::span<const std::byte> file) {
    constexpr std::string_view magic = "GYMIMG01";
    if (file.size() < kHeader) return;
    for (std::size_t i = 0; i < magic.size(); ++i)
        if (static_cast<char>(file[i]) != magic[i]) return;
    const int w = static_cast<int>(read_le(file, 8, 2)), h = static_cast<int>(read_le(file, 10, 2));
    const std::size_t n = read_le(file, 12, 4);
    if (w <= 0 || h <= 0 || file.size() < kHeader + 4 * n) return;
    data_ = file;
    width_ = w;
    height_ = h;
    count_ = static_cast<int>(n);
}

std::optional<Image> ImagePack::frame(int exercise, int frame) const {
    if (exercise < 0 || exercise >= count_ || frame < 0 || frame >= kFrames) return std::nullopt;
    const std::size_t offset = read_le(data_, kHeader + 4 * static_cast<std::size_t>(exercise), 4);
    if (offset == 0) return std::nullopt;
    const std::size_t bytes = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 2;
    const std::size_t start = offset + bytes * static_cast<std::size_t>(frame);
    if (start + bytes > data_.size()) return std::nullopt;
    return Image{width_, height_, data_.subspan(start, bytes)};
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::vector<char> raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<std::byte> out(raw.size());
    std::ranges::transform(raw, out.begin(), [](char c) { return static_cast<std::byte>(c); });
    return out;
}

}  // namespace gym
