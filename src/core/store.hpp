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
//   plan <name>
//   item <exercise index> <sets> <reps>        (belongs to the last plan)
//   day <0-6, Monday = 0> <plan index>
// Files from older versions (only session and set lines) still load.
std::string serialize(const Data& data);
std::expected<Data, std::string> parse(std::string_view text);

bool save_file(const std::filesystem::path& path, const Data& data);
// Returns empty data if the file is missing or damaged.
Data load_file(const std::filesystem::path& path);

}  // namespace gym
