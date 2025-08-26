from typing import OrderedDict
from esphome import pins
from esphome.const import CONF_ID, CONF_RX_PIN, CONF_TX_PIN, CONF_DISCOVERY
from esphome.core import CORE

import esphome.codegen as cg
import esphome.config_validation as cv

AUTO_LOAD = ["light", "output"]

CONF_DALI_BUS = 'dali_bus'
CONF_INITIALIZE_ADDRESSES = 'initialize_addresses'

dali_ns = cg.esphome_ns.namespace('dali')
dali_lib_ns = cg.global_ns
DaliBusComponent = dali_ns.class_('DaliBusComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DaliBusComponent),
    cv.Required(CONF_RX_PIN): pins.gpio_input_pin_schema,
    cv.Required(CONF_TX_PIN): pins.gpio_output_pin_schema,
    cv.Optional(CONF_DISCOVERY): cv.All(cv.requires_component("light"), cv.boolean),
    cv.Optional(CONF_INITIALIZE_ADDRESSES): cv.boolean,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config: OrderedDict):
    var = cg.new_Pvariable(config[CONF_ID])
    bus = await cg.register_component(var, config)

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(rx_pin))
    
    tx_pin = await cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_tx_pin(tx_pin))

    if config.get(CONF_DISCOVERY, False):
        cg.add(var.do_device_discovery())

        # When discovery is enabled but no light components are defined
        # in the YAML, we need to make it look like we have a light 
        # defined so it will compile in support. Without this, USE_LIGHT
        # will not be defined.
        #
        # This can be done by registering this bus component as a light,
        # making the core think there is at least one light defined.
        CORE.register_platform_component("light", bus)

    if config.get(CONF_INITIALIZE_ADDRESSES, False):
        cg.add(var.do_initialize_addresses())
