
#include <esphome.h>
#include "esphome_dali_light.h"
#include "esphome/core/log.h"

using namespace esphome;
using namespace dali;

using namespace esphome::light;

static const char *const TAG = "dali.light";

#define DALI_MAX_BRIGHTNESS_F (254.0f)

// File-static instance pointer for the set_initial_state trampoline.
// Safe because ESPHome setup is single-threaded: each LightState::setup()
// calls setup_state() then invokes the callback in the same frame.
static DaliLight* s_setup_instance = nullptr;

void DaliLight::initial_state_trampoline(LightStateRTCState &s) {
    if (s_setup_instance) {
        if (s_setup_instance->initial_has_brightness_) {
            s.brightness = s_setup_instance->initial_brightness_;
        }
        if (s_setup_instance->initial_has_color_temp_) {
            s.color_temp = s_setup_instance->initial_color_temp_;
        }
    }
}

void dali::DaliLight::setup_state(light::LightState *state) {
    // Initialization code for DaliLight

    // Exclude broadcast and group addresses
    if ((this->address_ != ADDR_BROADCAST) && ((this->address_ & ADDR_GROUP_MASK) == 0)) {
        ESP_LOGD(TAG, "Querying DALI device capabilities...");
        if (bus->dali.isDevicePresent(address_)) {
            ESP_LOGD(TAG, "DALI[%.2x] Is Present", address_);

            this->dali_level_min_ = bus->dali.lamp.getMinLevel(address_);
            this->dali_level_max_ = bus->dali.lamp.getMaxLevel(address_);
            this->dali_level_range_ = (float)(dali_level_max_ - this->dali_level_min_ + 1);
            ESP_LOGD(TAG, "Reported min:%d max:%d", this->dali_level_min_, this->dali_level_max_);

            // NOTE: Some DALI controllers report their device type is LED(6) even though they do also support color temperature,
            // so let's explicitly check if they respond to this:
            this->tc_supported_ = bus->dali.color.isTcCapable(address_);
            if (tc_supported_) {
                ESP_LOGD(TAG, "DALI[%.2x] Supports color temperature", address_);

                // TODO: Don't seem to be getting the full range here?
                // Tc(cool)=153, Tc(warm)=370
                uint16_t coolest = bus->dali.color.queryParameter(address_, DaliColorParam::ColourTemperatureTcCoolest);
                uint16_t warmest = bus->dali.color.queryParameter(address_, DaliColorParam::ColourTemperatureTcWarmest);

                ESP_LOGD(TAG, "Tc(cool)=%d, Tc(warm)=%d", coolest, warmest);
                if (coolest > COLOR_MIREK_WARMEST || warmest > COLOR_MIREK_WARMEST) {
                    ESP_LOGW(TAG, "Tc min/max is out of range!");
                } else {
                    // Store reported coolest/warmest mired values for mapping.
                    // NOTE: Not updating the configuration-provided warm/cool values, those are for UI only.
                    // Ultimately we don't really want to trust the mired range reported by the dimmer
                    // as it depends on the LED strip attached. So we map the UI range into the reported range.
                    this->dali_tc_coolest_ = (float)coolest;
                    //this->dali_tc_warmest_ = (float)warmest;
                }
            }
            else {
                ESP_LOGD(TAG, "Does not support color temperature");
            }

            ESP_LOGD(TAG, "Sending configuration to device...");

            if (this->brightness_curve_.has_value()) {
                switch (this->brightness_curve_.value()) {
                    case DaliLedDimmingCurve::LOGARITHMIC: ESP_LOGD(TAG, "Setting brightness curve to LOGARITHMIC"); break;
                    case DaliLedDimmingCurve::LINEAR:      ESP_LOGD(TAG, "Setting brightness curve to LINEAR"); break;
                }
                bus->dali.led.setDimmingCurve(address_, this->brightness_curve_.value());
            }

            if (this->fade_rate_.has_value()) {
                ESP_LOGD(TAG, "Setting fade rate: %d", this->fade_rate_.value());
                bus->dali.lamp.setFadeRate(0, this->fade_rate_.value());
            }
            if (this->fade_time_.has_value()) {
                ESP_LOGD(TAG, "Setting fade time: %d", this->fade_time_.value());
                bus->dali.lamp.setFadeTime(0, this->fade_time_.value());
            }

            // bus->dali.lamp.setMinLevel(address_, 1);
            // bus->dali.lamp.setMaxLevel(address_, 254);

            // Query the actual brightness level of the device and 
            // ensure this is reflected in the ESPHome component itself...
            uint8_t current_level = bus->dali.lamp.getCurrentLevel(address_);
            if (current_level != 0) {
                // TODO: Do we need to take into account reported min/max brightness?
                initial_brightness_ = (current_level * (1.0f/255.0f));
                initial_has_brightness_ = true;
                ESP_LOGD(TAG, "Restore brightness level: %.2f", initial_brightness_);
            }

            if (tc_supported_) {
                uint16_t current_temperature = bus->dali.color.getColorTemperature(address_);
                if (current_temperature != 0) {
                    float mired = (float)current_temperature;
                    // Need to convert mireds to 0..1 range
                    initial_color_temp_ = (current_temperature - dali_tc_coolest_) / (dali_tc_warmest_ - dali_tc_coolest_);
                    initial_has_color_temp_ = true;
                    ESP_LOGD(TAG, "Restore colour temperature: %.2f", initial_color_temp_);
                }
            }

            s_setup_instance = this;
            state->set_initial_state(initial_state_trampoline);
        }
        else {
            ESP_LOGW(TAG, "DALI device at addr %.2x not found!", address_);
        }

        //bus->dali.dumpStatusForDevice(address_);
    }
    else {
        // TODO: How do we detect color temperature support for broadcast and group addresses?
    }


    // if (this->color_mode_.has_value()) {
    //     if (this->color_mode_.value() == DaliColorMode::COLOR_TEMPERATURE) {
    //         tc_supported_ = true;
    //         ESP_LOGD(TAG, "Override: enable color temperature support");
    //     } else {
    //         tc_supported_ = false;
    //         ESP_LOGD(TAG, "Override: disable color temperature support");
    //     }
    // }
}

light::LightTraits dali::DaliLight::get_traits() {
    light::LightTraits traits;

    // NOTE: This is called repeatedly, do not perform any bus queries here...

    // Force a color mode irrespective of what the device itself says it supports
    // eg. you can convert a CT capable device to a plain brighness device,
    // or force colour temperature support and hope the device recognizes the command...
    if (this->color_mode_.has_value()) {
        switch (this->color_mode_.value()) {
            case DaliColorMode::COLOR_TEMPERATURE: 
                this->tc_supported_ = true;
                traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
                traits.set_min_mireds(this->cold_white_temperature_);
                traits.set_max_mireds(this->warm_white_temperature_);
                break;
            case DaliColorMode::BRIGHTNESS:
                this->tc_supported_ = false;
                traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
                break;
            case DaliColorMode::ON_OFF:
                this->tc_supported_ = false;
                traits.set_supported_color_modes({light::ColorMode::ON_OFF});
                break;
        }
    }
    else {
        // Device reports color temperature support
        if (this->tc_supported_) {
            traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
            traits.set_min_mireds(this->cold_white_temperature_);
            traits.set_max_mireds(this->warm_white_temperature_);
        }
        else {
            traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
        }
    }

    return traits;
}

void dali::DaliLight::write_state(light::LightState *state) {
    bool on;
    float brightness;
    float color_temperature;

    static uint16_t last_temperature = 0;

    state->current_values_as_binary(&on);
    if (!on) {
        // Short cut: send power off command
        //bus->dali.lamp.turnOff(address_); // no fade
        bus->dali.lamp.setBrightness(address_, 0); // fade
        return;
    }

    if (tc_supported_) {
        state->current_values_as_ct(&color_temperature, &brightness);

        // Map temperature 0..1 to reported TC coolest/warmest mireds
        // NOTE: Not using the configuration warm/cool colours - these may not match the reported range of the DALI device.
        float color_temperature_mired = (color_temperature * (dali_tc_warmest_ - dali_tc_coolest_)) + dali_tc_coolest_;

        uint16_t dali_color_temperature = static_cast<uint16_t>(color_temperature_mired);

        // Only update if temperature has changed, to allow faster brightness changes
        if (dali_color_temperature != last_temperature) {
            last_temperature = dali_color_temperature;

            ESP_LOGD(TAG, "DALI[%d] Tc=%d", address_, dali_color_temperature);

            // IMPORTANT: Do not set start_fade (activate), or the color temperature fade will
            // be cancelled when we next call setBrightness, and no color change will occur.
            bus->dali.color.setColorTemperature(address_, dali_color_temperature, false);
        }
    } else {
        state->current_values_as_brightness(&brightness);
    }

    int dali_brightness = static_cast<uint8_t>(brightness * this->dali_level_range_) + this->dali_level_min_ - 1;
    if (dali_brightness < 1) dali_brightness = 1;
    if (dali_brightness > 254) dali_brightness = 254;

    ESP_LOGD(TAG, "DALI[%d] B=%.2f (%d)", address_, brightness, dali_brightness);
    bus->dali.lamp.setBrightness(address_, (uint8_t)dali_brightness);
}
