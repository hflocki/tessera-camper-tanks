"""Camper tanks: one tile type that draws up to four tanks.

The screen reads each tank's level and content itself (homeassistant sensors of plugin.yaml), with its kind and name
as settings. register_plugin() takes the id, version, tile memory and the screen's texts from the manifest and
translations/ beside this folder.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select, sensor, smart_display, text, text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["smart_display"]
AUTO_LOAD = ["sensor", "text_sensor", "select", "text"]

MAX_TANKS = 4

camper_tanks_ns = cg.esphome_ns.namespace("camper_tanks")
CamperTanks = camper_tanks_ns.class_("CamperTanks", cg.Component)

TANK_SCHEMA = cv.Schema(
    {
        cv.Required("level"): cv.use_id(sensor.Sensor),
        cv.Optional("amount"): cv.use_id(sensor.Sensor),
        cv.Optional("ha_name"): cv.use_id(text_sensor.TextSensor),
        cv.Optional("type"): cv.use_id(select.Select),
        cv.Optional("name"): cv.use_id(text.Text),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(CamperTanks),
        cv.Optional("tanks", default=[]): cv.All(cv.ensure_list(TANK_SCHEMA), cv.Length(max=MAX_TANKS)),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await smart_display.register_plugin(var, __file__)
    for index, tank in enumerate(config["tanks"]):
        cg.add(var.set_level(index, await cg.get_variable(tank["level"])))
        if "amount" in tank:
            cg.add(var.set_amount(index, await cg.get_variable(tank["amount"])))
        if "ha_name" in tank:
            cg.add(var.set_ha_name(index, await cg.get_variable(tank["ha_name"])))
        if "type" in tank:
            cg.add(var.set_type(index, await cg.get_variable(tank["type"])))
        if "name" in tank:
            cg.add(var.set_name(index, await cg.get_variable(tank["name"])))
