#include "dali_output.h"
#include "esphome.h"

using namespace esphome;
using namespace dali;

static const char *const TAG = "dali.output";

void DaliOutput::setup() {

}

void DaliOutput::loop() {

}

void DaliOutput::write_state(float state) {
    if (bus == nullptr) {
        return;
    }

    // Convert the state to uint8 brightness
    uint8_t level = static_cast<uint8_t>(state * 255.0f);
    if (level > 254) {
        level = 254;
    }
    if (level < 0) {
        level = 0;
    }

    // Broadcast the frame to the DALI bus
    bus->dali.lamp.setBrightness(ADDR_BROADCAST, level);
}
