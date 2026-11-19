#pragma once

#include <string>
#include <unordered_map>

namespace emu_ui {

/// The save states in `dir` per game, as both boards name them:
/// "<rom stem>_<slot><extension>.sav" (extension: "", "_nes", "_gb" ...;
/// a slot's screenshot, ".sav.bin", is not counted). Keyed by the rom
/// file's stem (std::filesystem::path(rom_path).stem()); empty if `dir`
/// is missing.
std::unordered_map<std::string, int> count_saves(const std::string &dir);

} // namespace emu_ui
