#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

#include "core/model.hpp"

namespace gym {

// Text format, one item per line:
//   gym 1
//   session <unix seconds>
//   set <exercise index> <weight x10> <reps>
std::string serialize(const Log& log);
std::expected<Log, std::string> parse(std::string_view text);

bool save_file(const std::filesystem::path& path, const Log& log);
// Returns an empty log if the file is missing or damaged.
Log load_file(const std::filesystem::path& path);

}  // namespace gym
