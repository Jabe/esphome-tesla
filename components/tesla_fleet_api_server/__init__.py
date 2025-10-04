import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

try:
    from esphome.const import CONF_DISPLAY_NAME
except ImportError:  # pragma: no cover - older ESPHome versions
    CONF_DISPLAY_NAME = "display_name"

try:
    from esphome.const import CONF_VIN
except ImportError:  # pragma: no cover - older ESPHome versions
    CONF_VIN = "vin"

from ..tesla_ble_vehicle import TeslaBLEVehicle

CODEOWNERS = ["@Jabe"]
DEPENDENCIES = []

TESLA_FLEET_API_SERVER_NS = cg.esphome_ns.namespace("tesla_fleet_api_server")
TeslaFleetAPIServer = TESLA_FLEET_API_SERVER_NS.class_(
    "TeslaFleetAPIServer", cg.Component
)

CONF_VEHICLE = "vehicle"
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(TeslaFleetAPIServer),
        cv.Required(CONF_VEHICLE): cv.use_id(TeslaBLEVehicle),
        cv.Optional(CONF_VIN): cv.string,
        cv.Optional(CONF_DISPLAY_NAME, default="Tesla"): cv.string,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    vehicle = await cg.get_variable(config[CONF_VEHICLE])
    cg.add(var.set_vehicle(vehicle))

    if CONF_DISPLAY_NAME in config:
        cg.add(var.set_display_name(config[CONF_DISPLAY_NAME]))

    if CONF_VIN in config:
        cg.add(var.set_vin(config[CONF_VIN]))

