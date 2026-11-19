#pragma once

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "lvgl.h"

#include "emu-ui/rows.hpp"
#include "emu-ui/theme.hpp"
#include "emu-ui/widgets.hpp"

namespace emu_ui {

/// The in-game pause menu: the paused frame, the save slot with its
/// screenshot, resume / save / load / reset / quit, and the board's
/// settings, over the game's console background.
///
/// Keys: up / down move, left / right change the slot or a setting, A acts,
/// B resumes.
class PauseMenu {
public:
  enum class Action { RESUME, RESET, SAVE, LOAD, QUIT };

  struct Config {
    Metrics metrics{Metrics::large_1280x720()};
    lv_indev_t *keypad{nullptr};
    int slots{6}; ///< slots 0 .. slots - 1
    std::function<void(Action)> on_action{};
    /// The slot changed (the board loads its screenshot: set_slot_image())
    std::function<void(int slot)> on_slot{};
    /// Rows below the actions (volume, brightness, video ...)
    std::vector<Row> settings{};
  };

  explicit PauseMenu(Config config);
  ~PauseMenu();

  lv_obj_t *screen() const { return screen_; }
  /// Load the screen, focus Resume and give the keypad the group
  void show();

  /// The game being played: its name and console (the background)
  void set_game(const std::string &title, const std::string &console_key);
  /// The paused frame and the slot's screenshot (image sources, nullptr: none)
  void set_pause_image(const void *src);
  void set_slot_image(const void *src);
  int slot() const { return slot_; }
  void set_slot(int slot);
  void set_fps(float fps);
  void set_battery(int percent, bool charging);
  /// Re-read the settings (changed elsewhere)
  void refresh_settings();
  /// Do `action` as its row would (tests, the USB link)
  void click(Action action);

private:
  void build_ui();
  void update_slot_text();

  Config cfg_;
  const Metrics &m_;
  int slot_{0};
  lv_obj_t *screen_{nullptr};
  Backdrop backdrop_;
  lv_obj_t *title_{nullptr}, *fps_{nullptr}, *battery_{nullptr};
  lv_obj_t *pause_image_{nullptr}, *slot_image_{nullptr}, *slot_caption_{nullptr};
  HintBar hints_;
  std::unique_ptr<RowList> rows_;
  int slot_row_{-1};
};

/// Read a "[w:u16be][h:u16be][rgb565 little endian]" screenshot into
/// `storage` (no allocation: it must be big enough) and describe it in
/// `desc`; false if the file is missing, short or too big.
bool load_screenshot(const char *path, std::span<uint8_t> storage, lv_image_dsc_t &desc);

} // namespace emu_ui
