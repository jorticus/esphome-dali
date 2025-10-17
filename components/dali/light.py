from esphome.components import light, output
from esphome.const import (
    CONF_ID, 
    CONF_OUTPUT_ID, 
    CONF_ADDRESS, 
    CONF_BRIGHTNESS, 
    CONF_COLOR_TEMPERATURE,
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
    CONF_COLOR_MODE,
    CONF_DEFAULT_TRANSITION_LENGTH
)

import esphome.codegen as cg
import esphome.config_validation as cv
import math

from . import dali_ns, dali_lib_ns, CONF_DALI_BUS, DaliBusComponent

CONF_FADE_TIME = 'fade_time'
CONF_FADE_RATE = 'fade_rate'
CONF_BRIGHTNESS_CURVE = 'brightness_curve'
DEPENDENCIES = ['dali']

DaliLight = dali_ns.class_('DaliLight', light.LightOutput)

DaliColorMode = dali_ns.enum("DaliColorMode", is_class=True)
DALI_COLOR_MODES = {
    "AUTO": DaliColorMode.AUTO,
    "ON_OFF": DaliColorMode.ON_OFF,
    "BRIGHTNESS": DaliColorMode.BRIGHTNESS,
    "COLOR_TEMPERATURE": DaliColorMode.COLOR_TEMPERATURE,
}

# enum is defined in library dali.h
DaliLedDimmingCurve = dali_lib_ns.enum("DaliLedDimmingCurve", is_class=True)
DALI_BRIGHTNESS_CURVES = {
    "LOGARITHMIC": DaliLedDimmingCurve.LOGARITHMIC,
    "LINEAR": DaliLedDimmingCurve.LINEAR,
}

def fmt_time(ms):
    if ms == 0:
        return "0ms"
    if ms < 1000:
        return f"{ms}ms"
    ms = ms / 1000
    if ms < 60:
        return f"{ms:.1f}s"
    ms = ms / 60
    return f"{ms:.1f}m"

ALLOWABLE_FADE_TIMES = [
    # T = 1/2 * sqrt(2^fade_time) seconds
    fmt_time(int((math.sqrt(2**i)) / 2.0 * 1000)) for i in range(0, 16)
]

# [506000, 357796, 253000, 178898, 126500, 89449, 63250, 44724, 31625, 22362, 15812, 11181, 7906, 5590, 3953, 2795]
ALLOWABLE_FADE_RATES = [
    # F = 506 / sqrt(2^fade_rate) steps/second
    (int((506 / math.sqrt(2**i)) * 1000)) for i in range(1, 16)
]



def validate_fade_time(value):
    # Raw fade value as integer 0..15
    if isinstance(value, int):
        if value < 0:
            raise cv.Invalid("Fade time must be a positive integer")
        if value > 15:
            raise cv.Invalid("Fade time must be less than 15")
        return value

    # Parse as time period, convert to string, and compare against allowable set
    time = cv.positive_time_period_milliseconds(value)
    ms = time.total_milliseconds
    tstr = fmt_time(ms)
    for i, test in enumerate(ALLOWABLE_FADE_TIMES):
        if test == tstr:
            return i

    raise cv.Invalid(f"Fade time must be one of {ALLOWABLE_FADE_TIMES}")

def validate_fade_rate(value):
    if not isinstance(value, int):
        raise cv.Invalid("Fade time must be a positive integer")
    if value <= 0:
        # NOTE: 0 also not allowed for fade rate
        raise cv.Invalid("Fade time must be a positive integer")
    
    if value <= 15:
        return value
    
    for i, test in enumerate(ALLOWABLE_FADE_RATES):
        if test == value:
            return i
        
    raise cv.Invalid(f"Fade rate must be one of {ALLOWABLE_FADE_RATES}")


CONFIG_SCHEMA = light.LIGHT_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(DaliLight),

    cv.Optional(CONF_COLD_WHITE_COLOR_TEMPERATURE, default='10000K'): cv.color_temperature,
    cv.Optional(CONF_WARM_WHITE_COLOR_TEMPERATURE, default='2700K'): cv.color_temperature,

    cv.GenerateID(CONF_DALI_BUS): cv.use_id(DaliBusComponent),
    cv.Optional(CONF_ADDRESS): cv.int_,

    cv.Optional(CONF_COLOR_MODE): cv.enum(DALI_COLOR_MODES),
    cv.Optional(CONF_BRIGHTNESS_CURVE): cv.enum(DALI_BRIGHTNESS_CURVES),

    cv.Optional(CONF_FADE_TIME): validate_fade_time, # TimePeriod (ms, s, m)
    cv.Optional(CONF_FADE_RATE): validate_fade_rate, # Rate (steps/second)

    # cv.Optional(
    #     CONF_DEFAULT_TRANSITION_LENGTH, default="1s"
    # ): cv.positive_time_period_milliseconds,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    # DaliLight must be linked to DaliBusComponent
    parent = await cg.get_variable(config[CONF_DALI_BUS])

    # LightState must be linked to DaliLight (LightOutput)
    var = await light.new_light(config, parent)

    if CONF_ADDRESS in config:
        cg.add(var.set_address(config[CONF_ADDRESS]))

    if CONF_COLD_WHITE_COLOR_TEMPERATURE in config:
        cg.add(var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE]))
    if CONF_WARM_WHITE_COLOR_TEMPERATURE in config:
        cg.add(var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE]))

    if CONF_COLOR_MODE in config:
        cg.add(var.set_color_mode(config[CONF_COLOR_MODE]))
    if CONF_BRIGHTNESS_CURVE in config:
        cg.add(var.set_brightness_curve(config[CONF_BRIGHTNESS_CURVE]))

    if CONF_FADE_TIME in config:
        cg.add(var.set_fade_time(config[CONF_FADE_TIME]))
    if CONF_FADE_RATE in config:
        cg.add(var.set_fade_rate(config[CONF_FADE_RATE]))
