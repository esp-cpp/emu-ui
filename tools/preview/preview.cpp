// Render emu-ui's launcher and pause menu at the two boards' sizes into
// PPM files (the covers are placeholder gradients).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "lvgl.h"

#include "emu-ui/launcher.hpp"
#include "emu-ui/pause_menu.hpp"

namespace {

uint32_t g_tick = 0;
std::vector<uint16_t> g_fb;
int g_w = 0, g_h = 0;

void flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px) {
  const int w = lv_area_get_width(area);
  for (int y = area->y1; y <= area->y2; y++) {
    memcpy(&g_fb[y * g_w + area->x1], px + (y - area->y1) * w * 2, w * 2);
  }
  lv_display_flush_ready(disp);
}

void run(int ms) {
  for (int t = 0; t < ms; t += 10) {
    g_tick += 10;
    lv_timer_handler();
  }
}

void save(const std::string &path) {
  lv_obj_invalidate(lv_screen_active());
  run(1500);
  FILE *f = fopen(path.c_str(), "wb");
  fprintf(f, "P6\n%d %d\n255\n", g_w, g_h);
  for (auto p : g_fb) {
    const uint8_t rgb[3] = {uint8_t((p >> 11) << 3), uint8_t(((p >> 5) & 0x3F) << 2), uint8_t((p & 0x1F) << 3)};
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
  printf("%s\n", path.c_str());
}

// a placeholder cover: a diagonal two-color gradient with a band
lv_draw_buf_t *placeholder(int w, int h, uint32_t a, uint32_t b) {
  auto *buf = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
  for (int y = 0; y < h; y++) {
    auto *row = reinterpret_cast<uint16_t *>(buf->data + y * buf->header.stride);
    for (int x = 0; x < w; x++) {
      int t = (x + y) * 255 / (w + h);
      if (y > h * 6 / 10 && y < h * 7 / 10) t = 255 - t;
      auto ch = [&](int s) { return ((a >> s & 0xFF) * (255 - t) + (b >> s & 0xFF) * t) / 255; };
      row[x] = ((ch(16) >> 3) << 11) | ((ch(8) >> 2) << 5) | (ch(0) >> 3);
    }
  }
  return buf;
}

struct Item {
  const char *title, *key, *name, *short_name, *subtitle;
};
const Item kGames[] = {
    {"Mario Bros.", "nes", "NES", "NES", ""},
    {"Super Mario Bros. 3", "nes", "NES", "NES", "1 save"},
    {"The Legend of Zelda 2: the Adventure of Link", "nes", "NES", "NES", ""},
    {"Metroid", "nes", "NES", "NES", ""},
    {"Chrono Trigger", "snes", "Super Nintendo", "SNES", "3 saves"},
    {"Donkey Kong Country", "snes", "Super Nintendo", "SNES", "1 save"},
    {"Earthbound", "snes", "Super Nintendo", "SNES", ""},
    {"Super Mario RPG: Legend of the Seven Stars", "snes", "Super Nintendo", "SNES", ""},
    {"Super Mario World", "snes", "Super Nintendo", "SNES", "2 saves"},
    {"Link's Awakening", "gb", "Game Boy", "Game Boy", ""},
    {"Tetris: DX", "gb", "Game Boy", "Game Boy", ""},
    {"Pokemon Crystal", "gbc", "Game Boy Color", "GB Color", ""},
    {"Metroid Fusion", "gba", "Game Boy Advance", "GBA", ""},
    {"Sonic The Hedgehog", "sms", "Master System", "SMS", ""},
    {"Streets of Rage II", "gg", "Game Gear", "Game Gear", ""},
    {"Sonic The Hedgehog 3", "genesis", "Sega Genesis", "Genesis", "2 saves"},
    {"Toy Story", "genesis", "Sega Genesis", "Genesis", ""},
    {"Metal Gear 2: Solid Snake", "msx", "MSX", "MSX", ""},
    {"Metal Gear Solid", "playstation", "PlayStation", "PlayStation", "Memory card"},
    {"Star Wars: Dark Forces", "pc", "PC", "PC", ""},
    {"Doom II: Hell on Earth", "pc", "PC", "PC", ""},
    {"Celeste", "pico8", "PICO-8", "PICO-8", ""},
};

std::vector<emu_ui::Row> settings_rows(bool box) {
  static int volume = 60, brightness = 80, video = 1, mute = 0, resume = 1, usb = 0, haptic = 12, cpu = 0;
  std::vector<emu_ui::Row> rows;
  using K = emu_ui::Row::Kind;
  rows.push_back({.label = "Volume", .kind = K::SLIDER, .get = [] { return volume; }, .set = [](int v) { volume = v; },
                  .symbol = LV_SYMBOL_VOLUME_MAX});
  rows.push_back({.label = "Mute", .kind = K::TOGGLE, .get = [] { return mute; }, .set = [](int v) { mute = v; },
                  .symbol = LV_SYMBOL_MUTE});
  rows.push_back({.label = "Brightness", .kind = K::SLIDER, .min = 10, .get = [] { return brightness; },
                  .set = [](int v) { brightness = v; }, .symbol = LV_SYMBOL_EYE_OPEN});
  rows.push_back({.label = "Video", .kind = K::CHOICE, .choices = {"Original", "Fit", "Fill"},
                  .get = [] { return video; }, .set = [](int v) { video = v; }, .symbol = LV_SYMBOL_IMAGE});
  rows.push_back({.label = "Resume last save", .kind = K::TOGGLE, .get = [] { return resume; },
                  .set = [](int v) { resume = v; }, .symbol = LV_SYMBOL_REFRESH});
  rows.push_back({.label = "USB drive", .kind = K::TOGGLE, .get = [] { return usb; }, .set = [](int v) { usb = v; },
                  .note = [] { return std::string("The SD card as a USB drive on the USB-C port"); },
                  .symbol = LV_SYMBOL_USB});
  if (box) {
    rows.push_back({.label = "Haptic", .kind = K::NUMBER, .min = 1, .max = 123, .step = 1,
                    .get = [] { return haptic; }, .set = [](int v) { haptic = v; }, .symbol = LV_SYMBOL_BELL});
    rows.push_back({.label = "Genesis CPU", .kind = K::CHOICE, .choices = {"Normal", "125%"},
                    .get = [] { return cpu; }, .set = [](int v) { cpu = v; }, .symbol = LV_SYMBOL_CHARGE});
  }
  return rows;
}

void scenes(const emu_ui::Metrics &m, const std::string &out, const char *name, bool box) {
  g_w = m.width;
  g_h = m.height;
  g_fb.assign(g_w * g_h, 0);
  auto *disp = lv_display_create(g_w, g_h);
  static std::vector<uint8_t> buf;
  buf.assign(g_w * g_h * 2, 0);
  lv_display_set_buffers(disp, buf.data(), nullptr, buf.size(), LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_flush_cb(disp, flush);
  lv_display_set_default(disp);
  lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
  auto *keypad = lv_indev_create();
  lv_indev_set_type(keypad, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(keypad, [](lv_indev_t *, lv_indev_data_t *d) { d->state = LV_INDEV_STATE_RELEASED; });

  const int cw = box ? 100 : 360, ch = box ? 136 : 360;
  std::vector<lv_draw_buf_t *> covers;
  const uint32_t palette[][2] = {{0x2F7D3A, 0xC9A227}, {0x7A1F3D, 0xE0662B}, {0x1F4F8A, 0x4FB0E0},
                                 {0x8A6A1F, 0xE0C24F}, {0x5A2F8A, 0xB04FE0}};
  for (int i = 0; i < 5; i++) covers.push_back(placeholder(cw, box ? 130 : ch, palette[i][0], palette[i][1]));

  std::vector<emu_ui::Game> games;
  for (const auto &g : kGames) games.push_back({g.title, g.subtitle, g.key, g.name, g.short_name});

  {
    emu_ui::Launcher launcher({
        .metrics = m,
        .brand = "tab5-emu",
        .keypad = keypad,
        .cover = [&](size_t i) -> const void * { return covers[i % covers.size()]; },
        .settings = settings_rows(box),
        .version = "v1.1.0  ota_0",
    });
    launcher.set_games(games, 5);
    launcher.set_battery(87, false);
    launcher.show();
    save(out + "/" + name + "-launcher.ppm");
    launcher.select(15);
    save(out + "/" + name + "-launcher-genesis.ppm");
    launcher.select(7);
    launcher.set_filter("snes");
    save(out + "/" + name + "-launcher-filtered.ppm");
    launcher.set_filter(std::nullopt);
    launcher.select(9);
    launcher.open_settings();
    save(out + "/" + name + "-settings.ppm");
    launcher.close_settings();
    run(50);
    launcher.open_picker();
    save(out + "/" + name + "-picker.ppm");
    launcher.close_picker();
    run(50);
    // an empty card
    launcher.set_games({}, -1);
    save(out + "/" + name + "-empty.ppm");
  }
  {
    auto *frame = placeholder(box ? 128 : 320, box ? 96 : 240, 0x1F4F8A, 0x4FB0E0);
    auto *slot = placeholder(box ? 128 : 320, box ? 96 : 240, 0x7A1F3D, 0xE0662B);
    emu_ui::PauseMenu menu({.metrics = m, .keypad = keypad, .settings = settings_rows(box)});
    menu.set_game("Sonic The Hedgehog 3", "genesis");
    menu.set_pause_image(frame);
    menu.set_slot(2);
    menu.set_slot_image(slot);
    menu.set_fps(59.9f);
    menu.set_battery(64, true);
    menu.show();
    save(out + "/" + name + "-pause.ppm");
    menu.set_slot_image(nullptr);
    menu.set_game("Link's Awakening", "gb");
    save(out + "/" + name + "-pause-empty-slot.ppm");
    {
      emu_ui::PauseMenu m2({.metrics = m, .keypad = keypad, .settings = settings_rows(box)});
      m2.set_game("Legend of Zelda, The - Link's Awakening DX (USA, Europe) (Rev 2) (SGB Enhanced)", "gbc");
      m2.show();
      for (int t = 0; t < 10; t++) { g_tick += 10; lv_timer_handler(); }
      m2.set_battery(100, true);
      m2.set_fps(59.7f);
      save(out + "/" + name + "-pause-long.ppm");
    }
  }
  lv_display_delete(disp);
}

} // namespace

int main(int argc, char **argv) {
  const std::string out = argc > 1 ? argv[1] : ".";
  lv_init();
  lv_tick_set_cb([] { return g_tick; });
  scenes(emu_ui::Metrics::large_1280x720(), out, "tab5", false);
  scenes(emu_ui::Metrics::compact_320x240(), out, "box", true);
  return 0;
}
