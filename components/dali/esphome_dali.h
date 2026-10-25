#pragma once

#include <esphome.h>
#include <vector>
#include "dali.h"

namespace esphome {
namespace dali {

enum class DaliInitMode {
    DiscoverOnly,
    InitializeUnassigned,
    InitializeAll
};

class DaliBusComponent : public Component, public DaliPort {
public:
    DaliBusComponent()
        : Component { }
        , dali { *this }
    { }

    void setup() override;
    void loop() override;
    void dump_config() override;

    void set_tx_pin(InternalGPIOPin* tx_pin) { m_txPin = tx_pin; }
    void set_rx_pin(InternalGPIOPin* rx_pin) { m_rxPin = rx_pin; }
    void set_invert_tx(bool invert) { m_invertTx = invert; }
    void set_invert_rx(bool invert) { m_invertRx = invert; }

    /// @brief Perform automatic device discovery on setup.
    /// Light components will automatically be created and appear in HomeAssistant
    void do_device_discovery() { m_discovery = true; }

    /// @brief Initialize long and short addresses for devices on the bus.
    /// @param mode 
    //          InitializeUnassigned - only devices that do not yet have an assigned short address
    ///         InitializeAll - all devices on the bus
    /// @note
    void do_initialize_addresses(DaliInitMode mode = DaliInitMode::InitializeUnassigned) { m_initialize_addresses = mode; }

    // NOTE: Must have a higher priority number than the components that depend on this.
    // ie, this must be initialized first.
    float get_setup_priority() const override { return setup_priority::HARDWARE; }

    void register_static_addr(short_addr_t short_addr) {
        if (short_addr < ADDR_SHORT_MAX) {
            m_addresses[short_addr] = 0xFFFFFF;
        }
    }

    DaliMaster dali;

public: // DaliPort
    void resetBus() override;
    bool lastReplyReceived() const override { return m_lastReplyReceived; }
    void sendForwardFrame(uint8_t address, uint8_t data) override;
    uint8_t receiveBackwardFrame(unsigned long timeout_ms = 100) override;
    uint8_t transactForwardFrame(uint8_t address, uint8_t data, unsigned long timeout_ms = 100) override;

private:
    bool m_lastReplyReceived = false;
    uint8_t runTransaction(uint8_t address, uint8_t data, bool expect_response, unsigned long timeout_ms);
    static short_addr_t find_free_address(const bool* is_used);
    void register_discovered(short_addr_t short_addr, uint32_t long_addr);
    void create_light_component(short_addr_t short_addr, uint32_t long_addr);

    InternalGPIOPin* m_rxPin;
    InternalGPIOPin* m_txPin;
    bool m_invertTx{false};
    bool m_invertRx{true};

    bool m_discovery = false;
    DaliInitMode m_initialize_addresses = DaliInitMode::DiscoverOnly;
    uint32_t m_addresses[ADDR_SHORT_MAX+1] = {0};

    // Dynamic lights created during discovery are not in ESPHome's looping_components_
    // (that list is fixed at compile time). We drive their loop() manually.
    std::vector<light::LightState*> m_dynamic_lights;
};

}  // namespace dali
}  // namespace esphome
