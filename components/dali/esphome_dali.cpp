#include <esphome.h>
#include <esp_task_wdt.h>
#include <driver/gpio.h>
#include <algorithm>
#include <vector>
#include "esphome_dali.h"
#include "esphome_dali_light.h"
#include "esp_idf_dali_api.h"
#include "port.h"

using namespace esphome;
using namespace dali;

namespace {

class AppRegistrationAccessor : public esphome::Application {
public:
    using Application::register_component_;
};

class DynamicDaliLightState : public esphome::light::LightState {
public:
    using esphome::light::LightState::LightState;

    void configure_dynamic_entity(const char* name, const char* object_id, bool disabled_by_default) {
        uint32_t entity_fields = (static_cast<uint32_t>(disabled_by_default) << esphome::ENTITY_FIELD_DISABLED_BY_DEFAULT_SHIFT);
        this->configure_entity_(name, esphome::fnv1_hash_object_id(object_id, std::strlen(object_id)), entity_fields);
    }
};

}  // namespace

void DaliBusComponent::setup() {
    m_txPin->setup();
    m_rxPin->setup();
    const auto err = dali_init_with_inversion(
        static_cast<gpio_num_t>(m_rxPin->get_pin()),
        static_cast<gpio_num_t>(m_txPin->get_pin()),
        m_invertTx,
        m_invertRx);
    if (err != ESP_OK) {
        DALI_LOGE("Failed to initialize DALI RMT interface: %s", esp_err_to_name(err));
        this->mark_failed();
        return;
    }
    DALI_LOGI("DALI bus ready");

    // ESP-IDF's rmt_new_rx_channel() unconditionally enables the internal pull-up on the RX pin.
    // https://github.com/espressif/esp-idf/blob/v5.5.6/components/esp_driver_rmt/src/rmt_rx.c#L295
    // The DALI interface board has its own divider on RX, and the pull-up lifts the low level to ~1V,
    // so disable it again.
    // Consider your board's RX pin voltage divider when deciding on pull-up configuration.
    // ToDo: does this need to be configurable based on the board's RX pin circuitry?
    gpio_set_pull_mode(static_cast<gpio_num_t>(m_rxPin->get_pin()), GPIO_FLOATING);

    if (m_discovery) {
        // Optional: reset devices on the bus so we are in a known-good state.
        // Can help if devices are not responding to anything.
        if (false) {
            this->resetBus();
            esp_task_wdt_reset();
        }

        if (dali.bus_manager.isControlGearPresent()) {
            DALI_LOGD("Detected control gear on bus");
        } else {
            DALI_LOGE("No DALI control gear detected on bus!");
            return; // Unlikely to get anything from discovery if no one responds to this
        }

        // Probe every short address first, so we know which are in use before assigning any new ones.
        bool is_used[ADDR_SHORT_MAX+1];
        DALI_LOGI("Probing short addresses...");
        for (int i = 0; i <= ADDR_SHORT_MAX; i++) {
            esp_task_wdt_reset();
            // Retry once, a collision or glitch could make a present device look absent
            is_used[i] = dali.bus_manager.isControlGearPresent(i) || dali.bus_manager.isControlGearPresent(i);
            if (is_used[i]) {
                DALI_LOGI("  Device present @ %.2x", i);
                if (!m_addresses[i]) {
                    m_addresses[i] = 0xFFFFFF; // long address unknown until the scan
                    create_light_component(i, 0);
                }
            }
        }

        // Short addresses are all known from the probe. The scan is only needed to assign addresses
        // to gear that has none, or when asked to re-randomize everything.
        // 0xFD is the "unaddressed" broadcast: only gear without a short address answers.
        const bool unaddressed_gear = dali.bus_manager.isControlGearPresent(0x7E) || this->lastReplyReceived();
        if (!unaddressed_gear && this->m_initialize_addresses != DaliInitMode::InitializeAll) {
            DALI_LOGI("All devices have a short address, skipping scan");
        } else {
            if (this->m_initialize_addresses != DaliInitMode::DiscoverOnly) {
                if (this->m_initialize_addresses == DaliInitMode::InitializeAll) {
                    DALI_LOGI("Randomizing addresses for *all* DALI devices");
                    dali.bus_manager.initialize(ASSIGN_ALL); 
                } 
                else if (this->m_initialize_addresses == DaliInitMode::InitializeUnassigned) {
                    // Only randomize devices without an assigned short address
                    DALI_LOGI("Randomizing addresses for unassigned DALI devices");
                    dali.bus_manager.initialize(ASSIGN_UNINITIALIZED); 
                }

                dali.bus_manager.randomize();
                dali.bus_manager.terminate();

                // Seem to need a delay to allow time for devices to randomize...
                delayMilliseconds(50);
            }

            DALI_LOGI("Begin device discovery...");
            dali.bus_manager.startAddressScan(); // All devices
            if (this->m_initialize_addresses == DaliInitMode::DiscoverOnly) {
                // Devices must be in initialisation mode and have a valid random address for COMPARE to answer.
                // Randomizing does not alter short addresses.
                dali.bus_manager.randomize();
                delayMilliseconds(50);
            }

            // Keep track of short addresses to detect duplicates
            bool duplicate_detected = false;
            bool is_discovered[ADDR_SHORT_MAX+1];
            for (int i = 0; i <= ADDR_SHORT_MAX; i++) {
                is_discovered[i] = false;
            }

            short_addr_t short_addr = 0xFF;
            uint32_t long_addr = 0;
            std::vector<uint32_t> seen_long_addrs;
            while (dali.bus_manager.findNextAddress(short_addr, long_addr)) {
                // A device that cannot be withdrawn would otherwise be found forever
                if (std::find(seen_long_addrs.begin(), seen_long_addrs.end(), long_addr) != seen_long_addrs.end()) {
                    DALI_LOGW("  Device %.6x found again after withdraw, stopping scan", long_addr);
                    break;
                }
                seen_long_addrs.push_back(long_addr);
                delayMilliseconds(1); // yield to ESP stack
                esp_task_wdt_reset();

                if (short_addr <= ADDR_SHORT_MAX) {
                    DALI_LOGI("  Device %.6x @ %.2x", long_addr, short_addr);

                    // Duplicate detection
                    if (is_discovered[short_addr]) {
                        if (m_initialize_addresses == DaliInitMode::DiscoverOnly) {
                            DALI_LOGW("  WARNING: Duplicate short address detected!");
                            duplicate_detected = true;
                        }
                        else {
                            // Assign a new address for this
                            short_addr = find_free_address(is_used);
                            if (short_addr == 0xFF) {
                                DALI_LOGE("  No free short address left");
                                dali.bus_manager.withdrawCurrentDevice();
                                continue;
                            }
                            DALI_LOGD("  Duplicate short address detected, assigning a new address: %.2x", short_addr);

                            if (!dali.bus_manager.programShortAddress(short_addr)) {
                                DALI_LOGE("  Could not program short address");
                                dali.bus_manager.withdrawCurrentDevice();
                                short_addr = 0xFF;
                                continue;
                            }
                        }
                    }
                    else {
                        is_discovered[short_addr] = true;
                    }
                    is_used[short_addr] = true;

                    // Withdraw after address is confirmed (spec order: find → program → withdraw)
                    dali.bus_manager.withdrawCurrentDevice();

                    register_discovered(short_addr, long_addr);
                }
                else if (short_addr == 0xFF) {
                    if (m_initialize_addresses == DaliInitMode::DiscoverOnly) {
                        DALI_LOGI("  Device %.6x @ --", long_addr);
                        DALI_LOGW("  No short address assigned!");
                        dali.bus_manager.withdrawCurrentDevice();
                        continue;
                    }
                    else {
                        short_addr = find_free_address(is_used);
                        if (short_addr == 0xFF) {
                            DALI_LOGE("  No free short address left");
                            dali.bus_manager.withdrawCurrentDevice();
                            continue;
                        }
                        DALI_LOGI("  Assigning short address: %.2x", short_addr);

                        if (!dali.bus_manager.programShortAddress(short_addr)) {
                            DALI_LOGE("  Could not program short address");
                            dali.bus_manager.withdrawCurrentDevice();
                            short_addr = 0xFF;
                            continue;
                        }

                        // Withdraw after successful address programming
                        dali.bus_manager.withdrawCurrentDevice();

                        DALI_LOGI("  Device %.6x @ %.2x", long_addr, short_addr);
                        is_discovered[short_addr] = true;
                        is_used[short_addr] = true;
                        register_discovered(short_addr, long_addr);
                    }
                }
            }

            DALI_LOGD("No more devices found!");
            dali.bus_manager.endAddressScan();

            if (duplicate_detected) {
                DALI_LOGW("Duplicate short addresses detected on the bus!");
                DALI_LOGW("  Devices may report inconsistent capabilities.");
                DALI_LOGW("  You should fix your address assignments.");
            }
        }
    }

    // If no dynamic lights were discovered, disable loop() to avoid unnecessary CPU cycles.
    if (m_dynamic_lights.empty()) {
        this->disable_loop();
    }
}

short_addr_t DaliBusComponent::find_free_address(const bool* is_used) {
    for (int i = 0; i <= ADDR_SHORT_MAX; i++) {
        if (!is_used[i]) {
            return i;
        }
    }
    return 0xFF;
}

void DaliBusComponent::register_discovered(short_addr_t short_addr, uint32_t long_addr) {
    // Dynamic component creation (if not defined in YAML)
    if (m_addresses[short_addr]) {
        DALI_LOGD("  Ignoring, already defined");
        return;
    }
    // 0 means "unused" in m_addresses, but 0x000000 is a valid long address
    m_addresses[short_addr] = long_addr ? long_addr : 0xFFFFFF;
    create_light_component(short_addr, long_addr);
}

void DaliBusComponent::create_light_component(short_addr_t short_addr, uint32_t long_addr) {
#ifdef USE_LIGHT
    DaliLight* dali_light = new DaliLight { this };
    dali_light->set_address(short_addr);

    const int MAX_STR_LEN = 20;
    char* name = new char[MAX_STR_LEN];
    char* id = new char[MAX_STR_LEN];
    snprintf(name, MAX_STR_LEN, "DALI Light %d", short_addr);
    if (long_addr) {
        snprintf(id, MAX_STR_LEN, "dali_light_%.6x", long_addr);
    } else {
        snprintf(id, MAX_STR_LEN, "dali_light_s%.2u", short_addr);
    }
    // NOTE: Not freeing these strings, they will be owned by LightState.

    auto* light_state = new DynamicDaliLightState { dali_light };
    // set_component_source is codegen-only since ESPHome 2026.4 (uint8_t index into PROGMEM table);
    // dynamic components cannot participate, and it was cosmetic (log source tagging) only.
    light_state->configure_dynamic_entity(name, id, false);
    App.register_light(light_state);
    static_cast<AppRegistrationAccessor&>(App).register_component_(light_state);

    light_state->add_effects({});

    // Initialize the DaliLight with the LightState:
    // queries device capabilities (min/max level, color temperature support)
    dali_light->setup_state(light_state);

    // Lights registered during our setup() are never set up by the app, so LightState::setup()
    // (which applies the initial state read from the device) has to be run here.
    // DaliLight::setup_state() is idempotent, as LightState::setup() calls it again.
    light_state->setup();

    // Track for manual loop() driving — dynamic lights are not in ESPHome's
    // compile-time looping_components_ list so their loop() won't be called automatically.
    if (m_dynamic_lights.empty()) {
        this->enable_loop();  // only enable loop when we have lights to drive
    }
    m_dynamic_lights.push_back(light_state);

    DALI_LOGI("Created light component '%s' (%s)", name, id);
#else
    DALI_LOGE("Not compiled with light component. Add `light:` to YAML.");
#endif
}

void DaliBusComponent::loop() {
    for (auto* light : m_dynamic_lights) {
        light->loop();
    }
}

void DaliBusComponent::dump_config() {
    static const char *const TAG = "dali";
    ESP_LOGCONFIG(TAG, "DALI Bus:");
    LOG_PIN("  TX Pin: ", m_txPin);
    LOG_PIN("  RX Pin: ", m_rxPin);
    ESP_LOGCONFIG(TAG, "  Discovery: %s", m_discovery ? "enabled" : "disabled");
    ESP_LOGCONFIG(TAG, "  Control Gear: %s", dali.bus_manager.isControlGearPresent() ? "present" : "not present");
    bool any = false;
    for (int i = 0; i <= ADDR_SHORT_MAX; i++) {
        if (m_addresses[i] > 0) {
            if (!any) {
                ESP_LOGCONFIG(TAG, "  Addresses:");
                any = true;
            }
            ESP_LOGCONFIG(TAG, "    %.2u = %.6x", i, m_addresses[i]);
        }
    }
}

void DaliBusComponent::resetBus() {
    DALI_LOGW("DALI reset pulse is not available with the RMT transport");
}

void DaliBusComponent::sendForwardFrame(uint8_t address, uint8_t data) {
    this->runTransaction(address, data, false, 0);
}

uint8_t DaliBusComponent::receiveBackwardFrame(unsigned long timeout_ms) {
    (void) timeout_ms;
    DALI_LOGE("Backward frames must be received as part of a DALI transaction");
    return 0;
}

uint8_t DaliBusComponent::transactForwardFrame(uint8_t address, uint8_t data, unsigned long timeout_ms) {
    return this->runTransaction(address, data, true, timeout_ms);
}

uint8_t DaliBusComponent::runTransaction(uint8_t address, uint8_t data, bool expect_response, unsigned long timeout_ms) {
    (void) timeout_ms;
    m_lastReplyReceived = false;
    esp_task_wdt_reset(); // discovery runs long synchronous sequences of transactions
    DALI_LOGD("TX: %02X %02X", address, data);

    dali_addressType_t address_type;
    uint8_t dali_address;
    bool is_command = (address & 1) != 0;

    if (address == 0xFE || address == 0xFF) {
        address_type = DALI_ADDRESS_TYPE_BROADCAST;
        dali_address = 0;
    } else if (address >= 0xA0) {
        address_type = DALI_ADDRESS_TYPE_SPECIAL_CMD;
        dali_address = address;
    } else if ((address & 0x80) != 0) {
        address_type = DALI_ADDRESS_TYPE_GROUP;
        dali_address = (address >> 1) & 0x0F;
    } else {
        address_type = DALI_ADDRESS_TYPE_SHORT;
        dali_address = address >> 1;
    }

    int result = DALI_RESULT_NO_REPLY;
    const auto err = dali_transaction(
        address_type,
        dali_address,
        is_command,
        data,
        false,
        DALI_TX_TIMEOUT_DEFAULT_MS,
        expect_response ? &result : nullptr);
    if (err != ESP_OK) {
        DALI_LOGE("DALI RMT transaction failed: %s", esp_err_to_name(err));
        return 0;
    }

    m_lastReplyReceived = expect_response && result != DALI_RESULT_NO_REPLY;
    if (!expect_response) {
        DALI_LOGD("RX: -- (not requested)");
        return 0;
    }
    if (result == DALI_RESULT_NO_REPLY) {
        DALI_LOGD("RX: -- (no reply)");
        return 0;
    }

    DALI_LOGD("RX: %02X", result);
    // Gear ignores a forward frame that follows its backward frame too closely (>= 22 Te, ~9.2 ms)
    delayMilliseconds(15);
    return static_cast<uint8_t>(result);
}
