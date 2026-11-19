#include "emu-ui/saves.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

namespace emu_ui {

namespace {

bool all_digits(const std::string &s) {
  return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

} // namespace

std::unordered_map<std::string, int> count_saves(const std::string &dir) {
  std::unordered_map<std::string, int> counts;
  std::error_code ec;
  for (std::filesystem::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
    std::string name = it->path().filename().string();
    constexpr std::string_view ext = ".sav";
    if (name.size() <= ext.size() || name.compare(name.size() - ext.size(), ext.size(), ext) != 0) continue;
    name.resize(name.size() - ext.size());
    // "<stem>_<slot>" or "<stem>_<slot>_<core>"
    auto us = name.rfind('_');
    if (us == std::string::npos) continue;
    if (!all_digits(name.substr(us + 1))) {
      name.resize(us);
      us = name.rfind('_');
      if (us == std::string::npos || !all_digits(name.substr(us + 1))) continue;
    }
    if (us == 0) continue;
    counts[name.substr(0, us)]++;
  }
  return counts;
}

} // namespace emu_ui
