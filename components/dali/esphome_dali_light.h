#pragma once

#include <esphome.h>
#include "esphome/components/light/light_output.h"
#include "esphome_dali.h"

#include <type_traits>

namespace esphome {
namespace dali {

namespace compat {
// ESPHome 2026.10+ has LightState::set_state_callback(void(LightStateRTCState&, bool restored)).
// Older versions have set_initial_state(void(LightStateRTCState&)) and set_restore_mode().
template<typename T>
auto set_state_callback_impl(T *state, void (*cb)(light::LightStateRTCState &, bool), int)
    -> decltype(state->set_state_callback(cb), void()) {
    state->set_state_callback(cb);
}

// Enumerator 0 of the restore mode enum is DEFAULT_OFF; deduced so no name is looked up on new ESPHome.
template<typename T, typename R>
void set_restore_default_off(T *state, void (T::*)(R)) {
    state->set_restore_mode(static_cast<R>(0));
}

template<typename T>
void set_state_callback_impl(T *state, void (*cb)(light::LightStateRTCState &, bool), long) {
    static void (*s_cb)(light::LightStateRTCState &, bool) = nullptr;
    s_cb = cb;
    set_restore_default_off(state, &T::set_restore_mode);
    state->set_initial_state([](light::LightStateRTCState &s) {
        // Without a saved state the color mode is UNKNOWN
        bool restored = s.color_mode != light::ColorMode::UNKNOWN;
        s_cb(s, restored);
    });
}

// Only one callback is supported on old ESPHome (static storage); it is shared by all lights,
// so the callback must be the same function for every light.
inline void set_state_callback(light::LightState *state, void (*cb)(light::LightStateRTCState &, bool)) {
    set_state_callback_impl(state, cb, 0);
}
}  // namespace compat

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
        , dali_level_min_(1)
        , dali_level_max_(254)
        , dali_level_range_(254.0f)
        , color_mode_()
        , brightness_curve_()
    { }

    light::LightTraits get_traits() override;

    void setup_state(light::LightState *state) override;
    void write_state(light::LightState *state) override;

    void set_address(uint8_t address) { 
        address_ = address; 

        // Prevent dynamic creation of lights with this address
        // (when device discovery is enabled)
        if (this->address_ <= ADDR_SHORT_MAX) {
            bus->register_static_addr(address_);
        }
    }

    void set_cold_white_temperature(float cold_white_temperature) { cold_white_temperature_ = cold_white_temperature; }
    void set_warm_white_temperature(float warm_white_temperature) { warm_white_temperature_ = warm_white_temperature; }

    void set_color_mode(DaliColorMode color_mode) { color_mode_ = color_mode; }
    void set_brightness_curve(DaliLedDimmingCurve brightness_curve) { brightness_curve_ = brightness_curve; }

    void set_fade_time(uint16_t fade_time) { fade_time_ = fade_time; }
    void set_fade_rate(uint16_t fade_rate) { fade_rate_ = fade_rate; }

    // NOTE: Must have a lower priority number than the DALI bus component
    float get_setup_priority() const override { return setup_priority::DATA; }

    // Applies the state read from the device to the initial light state (called from per-address trampolines).
    static void apply_initial_state(DaliLight *self, light::LightStateRTCState &s, bool restored);

 protected:
    DaliBusComponent *bus;

    uint8_t address_;
    optional<uint16_t> fade_time_;
    optional<uint16_t> fade_rate_;

    float cold_white_temperature_;
    float warm_white_temperature_;
    float dali_tc_coolest_;
    float dali_tc_warmest_;
    uint8_t dali_level_min_;
    uint8_t dali_level_max_;
    float dali_level_range_;
    optional<DaliColorMode> color_mode_;
    optional<DaliLedDimmingCurve> brightness_curve_;

    bool tc_supported_;

    // Cached initial state from DALI device queries, used by the static
    // trampoline passed to set_initial_state() (ESPHome 2026.4+ callback API).
    float initial_brightness_{1.0f};
    float initial_color_temp_{1.0f};
    bool initial_has_brightness_{false};
    bool initial_has_color_temp_{false};
    bool initial_on_{false};
    bool initial_has_state_{false};
    bool setup_done_{false};
    
};

}  // namespace dali
}  // namespace esphome