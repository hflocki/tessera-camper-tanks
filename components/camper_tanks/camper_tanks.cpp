#include "camper_tanks.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace esphome::camper_tanks {

using tessera::Font;
using tessera::Tone;
namespace ui = tessera::ui;

// The kind of each tank when no select says otherwise (the select's initial options in plugin.yaml).
static const Kind DEFAULT_KIND[MAX_TANKS] = {Kind::FRESH, Kind::GAS, Kind::GREY, Kind::BLACK};
static const char *const KIND_KEY[] = {"fresh", "grey", "black", "gas", "diesel"};

// Shows or hides a part, only when that changes.
static void show(lv_obj_t *obj, bool shown) {
  if (!obj || lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) == !shown) return;
  if (shown) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

// The colour a kind's content is drawn in.
static theme::Role fill_role(Kind k) {
  switch (k) {
    case Kind::FRESH: return theme::MARK_BLUE;
    case Kind::GREY: return theme::MUTED;
    case Kind::BLACK: return theme::MARK_PURPLE;
    case Kind::GAS: return theme::MARK_GREEN;
    default: return theme::MARK_AMBER;
  }
}

// A number with the screen's decimal mark.
static std::string number_text(const tessera::Plugin *p, float v, int decimals) {
  char buf[16];
  snprintf(buf, sizeof buf, "%.*f", decimals, v);
  std::string s = buf;
  const std::string mark = p->text("decimal");
  const size_t dot = s.find('.');
  if (dot != std::string::npos && !mark.empty()) s.replace(dot, 1, mark);
  return s;
}

// The largest font whose line fits `h` and in which `sample` fits `w`.
static Font fitting_font(const std::string &sample, int w, int h) {
  for (Font f : {Font::DIGITS, Font::VALUE, Font::HEADLINE, Font::TITLE, Font::BODY_LARGE, Font::BODY}) {
    if (f == Font::DIGITS && !ui::covers(sample, f)) continue;
    if (ui::line_height(f) <= h && ui::text_width(sample, f) <= w) return f;
  }
  return Font::BODY;
}

Kind CamperTanks::kind(int i) const {
  const Source &t = tank(i);
  if (t.type) {
    auto index = t.type->active_index();
    if (index.has_value() && *index < 5) return static_cast<Kind>(*index);
  }
  return DEFAULT_KIND[ok(i) ? i : 0];
}

// The name a person gave the tank, else its name in Home Assistant, else "Tank n".
std::string CamperTanks::name(int i) const {
  const Source &t = tank(i);
  if (t.name && !t.name->state.empty()) return t.name->state;
  if (t.ha_name && t.ha_name->has_state() && !t.ha_name->state.empty()) return t.ha_name->state;
  return tessera::format(text("tank"), i + 1);
}

// ---------- One tank on a tile ----------
// A shell with its content rising from the bottom, three marks at a quarter, half and three quarters, and beside it
// or under it the percentage, the name and the content. A gas bottle is a capsule with a valve on top; diesel a
// canister with a spout.
class Slot {
 public:
  void make(lv_obj_t *parent, const tessera::Plugin *plugin, int tank, int x, int y, int w, int h, bool amount) {
    plugin_ = plugin;
    tank_ = tank;
    with_amount_ = amount;
    const int gap = ui::px(6);
    const int bh = ui::line_height(Font::BODY);
    const std::string sample = "100%";
    if (w >= h * 1.15f) {
      // Wide: the tank on the left, the words beside it.
      gw_ = std::min((int) (h * 0.85f), w * 42 / 100);
      gh_ = h;
      gx_ = x;
      gy_ = y;
      const int tx = x + gw_ + gap, tw = w - gw_ - gap;
      const int lines = amount ? 2 : 1;
      pct_font_ = fitting_font(sample, tw, std::max(bh, h - lines * bh));
      const int ph = ui::line_height(pct_font_), block = ph + lines * bh;
      const int ty = y + std::max(0, (h - block) / 2);
      make_labels(parent, tx, ty, tw, LV_TEXT_ALIGN_LEFT);
      lv_obj_set_pos(name_, tx, ty + ph);
      if (amount_) lv_obj_set_pos(amount_, tx, ty + ph + bh);
    } else {
      // Tall or square: the tank on top, the percentage and the name under it.
      const int lines = amount && h > ui::px(110) ? 2 : 1;
      with_amount_ = amount && lines == 2;
      pct_font_ = fitting_font(sample, w, std::max(bh, h * 30 / 100));
      const int ph = ui::line_height(pct_font_), words = ph + lines * bh;
      gh_ = std::max(ui::px(16), h - words - gap / 2);
      gw_ = std::min(w, (int) (gh_ * 0.85f));
      gx_ = x + (w - gw_) / 2;
      gy_ = y;
      const int ty = y + gh_ + gap / 2;
      make_labels(parent, x, ty, w, LV_TEXT_ALIGN_CENTER);
      lv_obj_set_pos(name_, x, ty + ph);
      if (amount_) lv_obj_set_pos(amount_, x, ty + ph + bh);
    }
    // The tank itself: its shape follows the kind in paint(), which may change while the tile shows.
    shell_ = ui::block(parent, theme::TRACK);
    lv_obj_set_style_clip_corner(shell_, true, 0);
    fill_ = ui::block(shell_, theme::MARK_BLUE);
    lv_obj_set_style_radius(fill_, 0, 0);
    for (auto *&m : mark_) {
      m = ui::block(shell_, theme::LINE);
      lv_obj_set_style_radius(m, 0, 0);
    }
    top_ = ui::block(parent, theme::TRACK);
    lv_obj_set_size(top_, 1, 1);
    show(top_, false);
  }

  // Draws the tank as its sensors say now.
  void paint(const CamperTanks *p, float low, float high) {
    const Source &t = p->tank(tank_);
    const Kind kind = p->kind(tank_);
    if (kind != kind_) shape(kind);
    const bool chosen = t.level && t.level->has_state();
    const float level = chosen ? t.level->state : NAN;
    const bool live = chosen && !std::isnan(level);

    for (auto *o : {shell_, pct_}) show(o, chosen);
    show(top_, chosen && has_top_);
    if (!chosen) {
      ui::set_text(name_, tessera::format(plugin_->text("not_chosen"), tank_ + 1));
      ui::set_color(name_, theme::MUTED);
      show(amount_, false);
      return;
    }
    ui::set_text(name_, p->name(tank_));
    ui::set_color(name_, theme::MUTED);

    const float pct = live ? std::max(0.0f, std::min(100.0f, level)) : 0;
    const bool waste = kind == Kind::GREY || kind == Kind::BLACK;
    const bool warn = live && (waste ? pct >= high : pct <= low);
    // The content, from the bottom of the shell.
    const int inner = lv_obj_get_height(shell_);
    const int fh = (int) std::lround(inner * pct / 100.0f);
    lv_obj_set_size(fill_, lv_obj_get_width(shell_), std::max(0, fh));
    lv_obj_set_pos(fill_, 0, inner - fh);
    show(fill_, live && fh > 0);
    ui::set_fill(fill_, warn ? Tone::ALERT : Tone::NORMAL, fill_role(kind));

    ui::set_text(pct_, live ? number_text(plugin_, pct, 0) + "%" : "--");
    if (warn) ui::set_tone(pct_, Tone::ALERT, theme::INK);
    else ui::set_color(pct_, live ? theme::INK : theme::MUTED);

    if (amount_) {
      const bool has = with_amount_ && t.amount && t.amount->has_state() && !std::isnan(t.amount->state);
      show(amount_, has);
      if (has) {
        const bool gas = kind == Kind::GAS;
        ui::set_text(amount_, tessera::fill(plugin_->text(gas ? "kg" : "liters"), "n",
                                            number_text(plugin_, t.amount->state, gas ? 1 : 0)));
        ui::set_color(amount_, theme::INK_SOFT);
      }
    }
  }

  // Light and dark: the colours that set_fill and set_color do not hold.
  void theme() {
    if (!shell_) return;
    lv_obj_set_style_bg_color(shell_, ui::color(theme::TRACK), 0);
    lv_obj_set_style_bg_color(top_, ui::color(theme::INK_SOFT), 0);
    for (auto *m : mark_) lv_obj_set_style_bg_color(m, ui::color(theme::LINE), 0);
  }

 private:
  void make_labels(lv_obj_t *parent, int x, int y, int w, lv_text_align_t align) {
    pct_ = ui::label(parent, pct_font_, theme::INK);
    name_ = ui::label(parent, Font::BODY, theme::MUTED);
    lv_obj_t *labels[3] = {pct_, name_, nullptr};
    if (with_amount_) amount_ = labels[2] = ui::label(parent, Font::BODY, theme::INK_SOFT);
    for (auto *l : labels) {
      if (!l) continue;
      lv_obj_set_width(l, w);
      lv_obj_set_style_text_align(l, align, 0);
    }
    lv_obj_set_pos(pct_, x, y);
  }

  // The shell, the marks and the top part for a kind of tank, in the graphic's room.
  void shape(Kind kind) {
    kind_ = kind;
    int w, h, x, y, tw = 0, th = 0, tx = 0;
    if (kind == Kind::GAS) {
      // A bottle: a capsule, and the valve on top in the middle.
      th = std::max(ui::px(3), gh_ * 12 / 100);
      h = gh_ - th;
      w = std::min(gw_, h * 55 / 100);
      tw = std::max(ui::px(4), w * 36 / 100);
      x = gx_ + (gw_ - w) / 2;
      y = gy_ + th;
      tx = x + (w - tw) / 2;
      lv_obj_set_style_radius(shell_, w / 2, 0);
    } else if (kind == Kind::DIESEL) {
      // A canister: a block with a spout at the top right.
      th = std::max(ui::px(3), gh_ * 10 / 100);
      h = gh_ - th;
      w = std::min(gw_, h * 75 / 100);
      tw = std::max(ui::px(4), w * 25 / 100);
      x = gx_ + (gw_ - w) / 2;
      y = gy_ + th;
      tx = x + w - tw - w / 8;
      lv_obj_set_style_radius(shell_, ui::px(4), 0);
    } else {
      // A water tank: a rounded block over the whole room.
      h = gh_;
      w = gw_;
      x = gx_;
      y = gy_;
      lv_obj_set_style_radius(shell_, std::max(ui::px(4), w / 8), 0);
    }
    lv_obj_set_pos(shell_, x, y);
    lv_obj_set_size(shell_, w, h);
    has_top_ = th > 0;
    show(top_, has_top_);
    if (th > 0) {
      lv_obj_set_pos(top_, tx, gy_);
      lv_obj_set_size(top_, tw, th + ui::px(2));
      lv_obj_set_style_radius(top_, ui::px(2), 0);
    }
    // Marks at a quarter, a half and three quarters, on the left inside the shell.
    const int mw = std::max(ui::px(4), w / 5), mh = std::max(1, ui::px(1));
    for (int i = 0; i < 3; ++i) {
      lv_obj_set_size(mark_[i], i == 1 ? mw * 3 / 2 : mw, mh);
      lv_obj_set_pos(mark_[i], kind == Kind::GAS ? w / 6 : ui::px(3), h - h * (i + 1) / 4);
    }
    theme();
  }

  const tessera::Plugin *plugin_{};
  int tank_ = 0, gx_ = 0, gy_ = 0, gw_ = 0, gh_ = 0;
  bool with_amount_ = true, has_top_ = false;
  Kind kind_ = static_cast<Kind>(255);
  Font pct_font_ = Font::BODY;
  lv_obj_t *shell_{}, *fill_{}, *mark_[3]{}, *top_{}, *pct_{}, *name_{}, *amount_{};
};

// ---------- The tile ----------
class TanksTile : public tessera::Tile {
 public:
  explicit TanksTile(const CamperTanks *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    // The tanks the inspector chose, in its order; none chosen: 1 to 4.
    int chosen[MAX_TANKS], n = 0;
    JsonArrayConst picks = c.options["tanks"].as<JsonArrayConst>();
    for (JsonVariantConst p : picks) {
      const char *v = p.is<const char *>() ? p.as<const char *>() : (p["value"] | "");
      const int t = atoi(v) - 1;
      if (t >= 0 && t < MAX_TANKS && n < MAX_TANKS && std::find(chosen, chosen + n, t) == chosen + n) chosen[n++] = t;
    }
    if (!n) for (int t = 0; t < MAX_TANKS; ++t) chosen[n++] = t;
    // One tank per cell.
    const int cells = std::max(1, c.columns * c.rows);
    count_ = std::min(n, cells);
    low_ = c.options["low"] | 15.0f;
    high_ = c.options["high"] | 85.0f;
    const bool amount = c.options["amount"] | true;

    // As many columns as the tile has, a row for the rest.
    const int cols = std::min(count_, std::max(1, (int) c.columns)), rows = (count_ + cols - 1) / cols;
    const int gap = ui::px(8);
    const int sw = (c.width - (cols - 1) * gap) / cols, sh = (c.height - (rows - 1) * gap) / rows;
    for (int i = 0; i < count_; ++i) {
      const int col = i % cols, row = i / cols;
      slots_[i].make(c.parent, plugin_, chosen[i], col * (sw + gap), row * (sh + gap), sw, sh, amount);
    }
  }

  void on_tick(uint32_t) override {
    for (int i = 0; i < count_; ++i) slots_[i].paint(plugin_, low_, high_);
  }

  void on_theme() override {
    for (int i = 0; i < count_; ++i) slots_[i].theme();
    on_tick(0);
  }

 private:
  const CamperTanks *plugin_;
  int count_ = 0;
  float low_ = 15, high_ = 85;
  Slot slots_[MAX_TANKS];
};

void CamperTanks::setup() {
  add_tile("tanks", [this]() { return new TanksTile(this); });
}

// The kind of each tank on the screen's own settings page; the names are changed in Home Assistant.
bool CamperTanks::settings(tessera::SettingsPage &page) {
  page.icon = ui::icon(0xF1A75);  // storage-tank
  std::vector<std::string> kinds;
  for (const char *k : KIND_KEY) kinds.push_back(text(k));
  bool any = false;
  for (int i = 0; i < MAX_TANKS; ++i) {
    select::Select *s = tanks_[i].type;
    if (!s) continue;
    any = true;
    const std::string label = tessera::format(text("type"), i + 1);
    page.choice(label.c_str(), kinds, [this, i] { return (int) kind(i); },
                [s](int v) {
                  auto call = s->make_call();
                  call.set_index(v);
                  call.perform();
                });
  }
  return any;
}

}  // namespace esphome::camper_tanks
