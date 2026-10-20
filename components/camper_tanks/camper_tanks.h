#pragma once
#include <cstdint>
#include <string>
#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text/text.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::camper_tanks {

static constexpr int MAX_TANKS = 4;

// What a tank holds, in the order of the select's options in plugin.yaml.
enum class Kind : uint8_t { FRESH, GREY, BLACK, GAS, DIESEL };

// One tank as the screen reads it from Home Assistant, with its two settings.
struct Source {
  sensor::Sensor *level{};
  sensor::Sensor *amount{};
  text_sensor::TextSensor *ha_name{};
  select::Select *type{};
  text::Text *name{};
};

// The plugin: one tile type ("tanks") and the kinds of the four tanks on the screen's settings page.
class CamperTanks : public Component, public tessera::Plugin {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  bool settings(tessera::SettingsPage &page) override;

  void set_level(int i, sensor::Sensor *s) { if (ok(i)) tanks_[i].level = s; }
  void set_amount(int i, sensor::Sensor *s) { if (ok(i)) tanks_[i].amount = s; }
  void set_ha_name(int i, text_sensor::TextSensor *s) { if (ok(i)) tanks_[i].ha_name = s; }
  void set_type(int i, select::Select *s) { if (ok(i)) tanks_[i].type = s; }
  void set_name(int i, text::Text *t) { if (ok(i)) tanks_[i].name = t; }

  const Source &tank(int i) const { return tanks_[ok(i) ? i : 0]; }
  Kind kind(int i) const;
  std::string name(int i) const;

 protected:
  static bool ok(int i) { return i >= 0 && i < MAX_TANKS; }
  Source tanks_[MAX_TANKS]{};
};

}  // namespace esphome::camper_tanks
