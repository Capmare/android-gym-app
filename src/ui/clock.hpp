#pragma once

#include <cstdint>
#include <string>

namespace gym {

std::int64_t epoch_now();                    // Unix time in seconds
std::string format_date(std::int64_t epoch); // "SUN 01 NOV 2026  10:52" in local time

}  // namespace gym
