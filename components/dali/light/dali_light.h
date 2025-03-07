#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/light/light_output.h"
#include "../esphome_dali.h"

namespace esphome {
namespace dali {

enum class DaliColorMode {
    AUTO,
    ON_OFF,
    BRIGHTNESS,
    COLOR_TEMPERATURE,
};

class DaliLight : public light::LightOutput, public Component {
 public:
    DaliLight(DaliBusComponent* parent)
        : bus(parent)
        , address_(ADDR_BROADCAST)
        , fade_time_()
        , fade_rate_()
        , cold_white_temperature_(100.0f) // 10000K
        , warm_white_temperature_(370.0f) // 2700K
        , tc_supported_(false)
        , dali_tc_coolest_(COLOR_MIREK_COOLEST)
        , dali_tc_warmest_(400.0f)
        , color_mode_()
        , brightness_curve_()
    { }

    light::LightTraits get_traits() override;

    void setup_state(light::LightState *state) override;
    void write_state(light::LightState *state) override;

    float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

    void set_address(uint8_t address) { address_ = address; }

    void set_cold_white_temperature(float cold_white_temperature) { cold_white_temperature_ = cold_white_temperature; }
    void set_warm_white_temperature(float warm_white_temperature) { warm_white_temperature_ = warm_white_temperature; }

    void set_color_mode(DaliColorMode color_mode) { color_mode_ = color_mode; }
    void set_brightness_curve(DaliLedDimmingCurve brightness_curve) { brightness_curve_ = brightness_curve; }

    void set_fade_time(uint16_t fade_time) { fade_time_ = fade_time; }
    void set_fade_rate(uint16_t fade_rate) { fade_rate_ = fade_rate; }

 protected:
    DaliBusComponent *bus;

    uint8_t address_;
    optional<uint16_t> fade_time_;
    optional<uint16_t> fade_rate_;

    output::FloatOutput *color_temperature_;
    output::FloatOutput *brightness_;
    float cold_white_temperature_;
    float warm_white_temperature_;
    float dali_tc_coolest_;
    float dali_tc_warmest_;
    optional<DaliColorMode> color_mode_;
    optional<DaliLedDimmingCurve> brightness_curve_;

    bool tc_supported_;
    
};

}  // namespace dali
}  // namespace esphome