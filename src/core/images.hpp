#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace gym {

// One photo: width * height pixels, row by row. Each pixel is 2 bytes, RGB565, little endian.
struct Image {
    int width{}, height{};
    std::span<const std::byte> rgb565;
};

// Reads the photo file made by tools/pack_images.py. The file has 2 photos per exercise
// (start and end of the movement). The pack only looks at the bytes. They must stay alive.
class ImagePack {
public:
    static constexpr int kFrames = 2;

    ImagePack() = default;
    explicit ImagePack(std::span<const std::byte> file);

    // Empty if the exercise has no photos or the file is damaged.
    std::optional<Image> frame(int exercise, int frame) const;
    bool has(int exercise) const { return frame(exercise, 0).has_value(); }
    int exercise_count() const noexcept { return count_; }

private:
    std::span<const std::byte> data_;
    int width_{}, height_{}, count_{};
};

// Reads a whole file. Empty if it cannot be read.
std::vector<std::byte> read_binary_file(const std::filesystem::path& path);

}  // namespace gym
