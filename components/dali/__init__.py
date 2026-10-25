from typing import OrderedDict

from esphome import pins
from esphome.components import esp32
from esphome.const import CONF_ID, CONF_RX_PIN, CONF_TX_PIN, CONF_DISCOVERY
from esphome.core import CORE

import esphome.codegen as cg
import esphome.config_validation as cv

AUTO_LOAD = ["light", "output"]

DALI_LIB_REPOSITORY = "https://github.com/ferdinandkeil/esp-idf-dali.git#master"

CONF_DALI_BUS = 'dali_bus'
CONF_INITIALIZE_ADDRESSES = 'initialize_addresses'
CONF_INVERT_TX = 'invert_tx'
CONF_INVERT_RX = 'invert_rx'

dali_ns = cg.esphome_ns.namespace('dali')
dali_lib_ns = cg.global_ns
DaliBusComponent = dali_ns.class_('DaliBusComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DaliBusComponent),
    cv.Required(CONF_RX_PIN): pins.internal_gpio_input_pin_schema,
    cv.Required(CONF_TX_PIN): pins.internal_gpio_output_pin_schema,
    cv.Optional(CONF_INVERT_TX, default=False): cv.boolean,
    cv.Optional(CONF_INVERT_RX, default=True): cv.boolean,
    cv.Optional(CONF_DISCOVERY): cv.All(cv.requires_component("light"), cv.boolean),
    cv.Optional(CONF_INITIALIZE_ADDRESSES): cv.Any(cv.boolean, cv.one_of('all', 'unassigned', lower=True)),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config: OrderedDict):
    cg.add_library("esp-idf-dali", None, DALI_LIB_REPOSITORY)
    esp32.include_builtin_idf_component("esp_driver_rmt")
    var = cg.new_Pvariable(config[CONF_ID])
    bus = await cg.register_component(var, config)

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(rx_pin))
    
    tx_pin = await cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_tx_pin(tx_pin))

    cg.add(var.set_invert_tx(config[CONF_INVERT_TX]))
    cg.add(var.set_invert_rx(config[CONF_INVERT_RX]))

    if config.get(CONF_DISCOVERY, False):
        cg.add(var.do_device_discovery())

        # When discovery is enabled but no light components are defined
        # in the YAML, we need to make it look like we have a light 
        # defined so it will compile in support. Without this, USE_LIGHT
        # will not be defined.
        #
        # This can be done by registering this bus component as a light,
        # making the core think there is at least one light defined.
        # The light list has a fixed capacity based on this count, and discovery
        # can create up to one light per short address (64).
        for _ in range(64):
            CORE.register_platform_component("light", bus)

    init = config.get(CONF_INITIALIZE_ADDRESSES, False)
    if init == 'all':
        cg.add(var.do_initialize_addresses(cg.RawExpression('dali::DaliInitMode::InitializeAll')))
    elif init:
        cg.add(var.do_initialize_addresses())
