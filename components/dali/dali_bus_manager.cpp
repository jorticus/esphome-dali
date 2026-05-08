#include "dali.h"
#include "port.h"


uint8_t DaliBusManager::autoAssignShortAddresses(uint8_t assign, bool reset) {
    if (reset) {
        DALI_LOGI("BEGIN AUTO ADDRESS ASSIGNMENT");
    } else {
        DALI_LOGI("BEGIN AUTO ADDRESS QUERY");
    }

    // Put matching devices into initialization mode
    initialize(assign);

    // If requested, randomize addresses before scanning
    if (reset) {
        DALI_LOGI("Randomizing addresses");
        randomize();
        delayMilliseconds(1000);
    }

    // Prepare for scanning without forcing a different initialize(assign) call
    this->_is_scanning = true;

    uint8_t count = 0;
    short_addr_t found_short;
    uint32_t found_long;

    while (findNextAddress(found_short, found_long)) {
        DALI_LOGD("Found long address: 0x%.6x", found_long);

        uint8_t program_short = (count << 1); // short address with command bit cleared
        uint8_t verify_short = 0xFF;

        if (reset) {
            // Program sequential short address (LSB is command bit, so keep it clear here)
            port.sendSpecialCommand(DaliSpecialCommand::PROGRAM_SHORT_ADDRESS, program_short | DALI_COMMAND);
            verify_short = program_short;
        } else {
            // Use any reported short address from device if available
            if (found_short != 0xFF) {
                verify_short = static_cast<uint8_t>(found_short << 1);
            } else {
                DALI_LOGW("Device 0x%.6x has no short address", found_long);
                verify_short = 0xFF;
            }
        }

        // Verify short address if we have one to verify
        if (verify_short != 0xFF) {
            port.sendSpecialCommand(DaliSpecialCommand::VERIFY_SHORT_ADDRESS, verify_short | DALI_COMMAND);
            if (port.receiveBackwardFrame() == 0xFF) {
                DALI_LOGD("Short address verified: %.2x", verify_short);
            } else {
                DALI_LOGE("Short address verification failed for 0x%.6x", found_long);
                delayMilliseconds(1000);
            }
        } else {
            DALI_LOGW("Skipping verification for 0x%.6x (no short address)", found_long);
        }

        count++;
    }

    // End scanning and exit initialization mode
    this->_is_scanning = false;
    terminate();

    if (count == 0) {
        DALI_LOGE("No devices found");
    }

    return count;
}

void DaliBusManager::startAddressScan() {
    if (!this->_is_scanning) {
        this->_is_scanning = true;
        // Put all devices on the bus into initialization mode, where they will accept special commands
        initialize(0);
    }
}

bool DaliBusManager::findNextAddress(short_addr_t& out_short_addr, uint32_t& out_long_addr) {
    if (!this->_is_scanning) {
        DALI_LOGE("Scan not started!");
        return false;
    }

    uint32_t addr = 0x000000; // Start with the lowest address

    // Shortcut: test if we are done
    if (!compareSearchAddress(0xFFFFFF)) {
        return false;
    }

    for (uint32_t i = 0; i < 24; i++) {
        uint32_t bit = 1ul << (uint32_t)(23ul - i);
        //uint32_t search_addr = addr | bit;
        addr |= bit;

        // True if actual address <= search_address
        bool compare_result = compareSearchAddress(addr);
        //DALI_LOGD("Test addr %.6x %.2x", addr, compare_result);
        if (compare_result) {
            addr &= ~bit; // Clear the bit
        } else {
            addr |= bit;  // Set the bit
        }
    }

    if (addr == 0xFFFFFF || addr == 0x000000) {
        return false; // No more devices found
    }

    // Final step in the search, set last bit if no longer matching
    if (!compareSearchAddress(addr)) {
        addr++;
    }

    // Sanity check: Address should still return true for comparison
    if (!compareSearchAddress(addr)) {
        DALI_LOGE("ERROR: Address did not match?");
        return false;
    }

    // NOTE: The caller is responsible for calling withdrawCurrentDevice() after any address programming,
    // per the DALI spec order: binary search → PROGRAM_SHORT_ADDRESS → WITHDRAW.
    _current_addr = addr;
    out_long_addr = addr;

    // Get short address
    port.sendSpecialCommand(DaliSpecialCommand::QUERY_SHORT_ADDRESS, 0);
    out_short_addr = port.receiveBackwardFrame();
    if (out_short_addr == 0) {
        DALI_LOGW("Short address not found for %.6x", addr);
        out_short_addr = 0xFF;
    }
    else if (out_short_addr <= ADDR_SHORT_MAX) {
        out_short_addr >>= 1; // remove command bit
    }

    return true;
}


void DaliBusManager::endAddressScan() {
    if (this->_is_scanning) {
        this->_is_scanning = false;
        // Exit initialization mode
        // Devices will respond to regular commands again
        terminate();
    }
}

void DaliBusManager::withdrawCurrentDevice() {
    withdraw(_current_addr);
}

void DaliMaster::dumpStatusForDevice(uint8_t addr) {
    DALI_LOGI("");
    DALI_LOGI("Device[%u]", addr); // Serial.print("] : 0x"); Serial.println(m_addresses[addr], HEX);
    bool present = port.sendQueryCommand(addr, DaliCommand::QUERY_CONTROL_GEAR_PRESENT);
    DALI_LOGI("  Present:      %s", present ? "YES" : "NO");

    if (!present) {
        return;
    }

    DALI_LOGI("  Version:      %02X", port.sendQueryCommand(addr, DaliCommand::QUERY_VERSION_NUMBER));
    //Serial.print("  Device Type:  "); Serial.println(port.sendQueryCommand(addr, CMD_QUERY_DEVICE_TYPE), HEX); FF -> ALL?

    uint8_t status = port.sendQueryCommand(addr, DaliCommand::QUERY_STATUS);
    DALI_LOGI("  Status:       %02X", status); //Serial.println(port.sendQueryCommand(addr, CMD_QUERY_STATUS), HEX);
        //if (status & STATUS_BALLAST_OK)             { Serial.println("    Ballast Not OK"); }
        if (status & STATUS_LAMP_FAILURE)           { DALI_LOGI("    Lamp Failure"); }
        if (status & STATUS_LAMP_ON)                { DALI_LOGI("    Lamp On"); } else { DALI_LOGI("    Lamp Off"); }
        if (status & STATUS_LIMIT_ERROR)            { DALI_LOGI("    Limit Error"); }
        if (status & STATUS_FADE_STATE)             { DALI_LOGI("    Fading"); }
        //if (status & STATUS_RESET_STATE)            { Serial.println("    Reset State"); }
        if (status & STATUS_MISSING_SHORT_ADDRESS)  { DALI_LOGI("    Missing Short Address"); }
        //if (status & STATUS_POWER_FAILURE)          { Serial.println("    Power Failure"); }

    DALI_LOGI("  Op Mode:      %02X", port.sendQueryCommand(addr, DaliCommand::QUERY_OPERATING_MODE));

    DALI_LOGI("  Lamp Failure: %02X", port.sendQueryCommand(addr, DaliCommand::QUERY_LAMP_FAILURE));
    DALI_LOGI("  Lamp Power:   %s", port.sendQueryCommand(addr, DaliCommand::QUERY_LAMP_POWER_ON) ? "ON" : "OFF");

    auto device_type = static_cast<DaliDeviceType>(port.sendQueryCommand(addr, DaliCommand::QUERY_LIGHT_SOURCE_TYPE));
    DALI_LOGI("  Light Type:   "); //Serial.println(port.sendQueryCommand(addr, CMD_QUERY_LIGHT_SOURCE_TYPE), HEX);
        switch (device_type) {
            case DaliDeviceType::FLUORESCENT: DALI_LOGI("Fluorescent"); break;
            case DaliDeviceType::EMERGENCY:    DALI_LOGI("Emergency"); break;
            case DaliDeviceType::HID:          DALI_LOGI("HID"); break;
            case DaliDeviceType::LV_HALOGEN:   DALI_LOGI("Halogen"); break;
            case DaliDeviceType::INCANDESCENT: DALI_LOGI("Incandescent"); break;
            case DaliDeviceType::DIGITAL:      DALI_LOGI("Digital"); break;
            case DaliDeviceType::LED:          DALI_LOGI("LED"); break;
            case DaliDeviceType::COLOR:        DALI_LOGI("Colour"); break;
            default: DALI_LOGI("%02X", static_cast<uint8_t>(device_type)); break;
        }

        if (device_type == DaliDeviceType::LED) {
            DALI_LOGI("    Extended Ver:   %02X", port.sendExtendedQuery(addr, DaliLedCommand::QUERY_EXTENDED_VERSION_NUMBER));
            DALI_LOGI("    Features:       %02X", port.sendExtendedQuery(addr, DaliLedCommand::QUERY_FEATURES));
            DALI_LOGI("    Dimming Curve:  %02X", port.sendExtendedQuery(addr, DaliLedCommand::QUERY_DIMMING_CURVE));
            DALI_LOGI("    Type:           %02X", port.sendExtendedQuery(addr, DaliLedCommand::QUERY_GEAR_TYPE));
            DALI_LOGI("    Failure:        %02X", port.sendExtendedQuery(addr, DaliLedCommand::QUERY_FAILURE_STATUS));
            DALI_LOGI("    Operating Mode: %02X", port.sendExtendedQuery(addr, DaliLedCommand::QUERY_OPERATING_MODE));
            DALI_LOGI("    Fast Fade Time: %02X", port.sendExtendedQuery(addr, DaliLedCommand::QUERY_FAST_FADE_TIME));

            uint8_t color_ver = port.sendExtendedQuery(addr, DaliColorCommand::QUERY_EXTENDED_VERSION_NUMBER);
            if (color_ver > 0) {
                DALI_LOGI("    -- Colour --");
                DALI_LOGI("    Extended Ver:   %02X", color_ver);
                uint8_t features = port.sendExtendedQuery(addr,DaliColorCommand::QUERY_COLOR_FEATURES);
                DALI_LOGI("    Features:       %02X", features);
                    if (features & COLOR_FEATURE_XY_CAPABLE) { DALI_LOGI("      XY Capable"); }
                    if (features & COLOR_FEATURE_TC_CAPABLE) { DALI_LOGI("      TC Capable"); }
                DALI_LOGI("    Status:         %02X", port.sendExtendedQuery(addr, DaliColorCommand::QUERY_COLOR_STATUS));

            }
        }

    DALI_LOGI("  Actual Level: %02X", port.sendQueryCommand(addr, DaliCommand::QUERY_ACTUAL_LEVEL));
    DALI_LOGI("  Max Level:    %02X", port.sendQueryCommand(addr, DaliCommand::QUERY_MAX_LEVEL));
    DALI_LOGI("  Min Level:    %02X", port.sendQueryCommand(addr, DaliCommand::QUERY_MIN_LEVEL));
    DALI_LOGI("  Power On Lvl: %02X", port.sendQueryCommand(addr, DaliCommand::QUERY_POWER_ON_LEVEL));

    DALI_LOGI("  Fade Time/Rate: %02X", port.sendQueryCommand(addr, DaliCommand::QUERY_FADE_TIME_FADE_RATE));
}
