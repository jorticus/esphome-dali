#pragma once

#include <stdint.h>

#if !defined(DALI_LOGD)
#if defined(ESPHOME_LOG_LEVEL)
#include "esphome/core/log.h"
static const char *const TAG_DALI = "dali";
#define DALI_LOGD(...) ESP_LOGD(TAG_DALI, __VA_ARGS__)
#define DALI_LOGI(...) ESP_LOGI(TAG_DALI, __VA_ARGS__)
#define DALI_LOGW(...) ESP_LOGW(TAG_DALI, __VA_ARGS__)
#define DALI_LOGE(...) ESP_LOGE(TAG_DALI, __VA_ARGS__)
#elif defined(DALI_ENABLE_STDIO_LOGGING)
#include <stdarg.h>
#include <stdio.h>
static inline void dali_stdio_log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
#define DALI_LOGD(...) dali_stdio_log(__VA_ARGS__)
#define DALI_LOGI(...) dali_stdio_log(__VA_ARGS__)
#define DALI_LOGW(...) dali_stdio_log(__VA_ARGS__)
#define DALI_LOGE(...) dali_stdio_log(__VA_ARGS__)
#else
#define DALI_LOGD(...)
#define DALI_LOGI(...)
#define DALI_LOGW(...)
#define DALI_LOGE(...)
#endif
#endif

/// @brief A 7-bit address for referencing a device on the bus.
/// @remark
///   Short address 0-63  0AAAAAA
///   Group address 0-15  100AAAA
///   Broadcast           1111111
/// Note: final 'S' bit is not to be set here
typedef uint8_t short_addr_t;

#define ADDR_BROADCAST (0x7F)       // 1111 111
#define ADDR_GROUP     (0x40)       // 100x xxx
#define ADDR_GROUP_MASK (0x70)       // 111x xxx
#define ADDR_SHORT_MAX  (63)

#define DALI_COMMAND    (0x01)
#define DALI_DIRECT_ARC (0x00)

#define ASSIGN_ALL           (0x00)
#define ASSIGN_UNINITIALIZED (0xFF)

enum class DaliCommand : uint8_t {
    OFF = 0x00, // Switches off lamp(s)
    UP = 0x01, // Increases lamp(s) illumination level
    DOWN = 0x02, // Decreases lamp(s) illumination level
    STEP_UP = 0x03, // Increases the target illumination level by 1
    STEP_DOWN = 0x04, // Decreases the target illumination level by 1
    RECALL_MAX_LEVEL = 0x05, // Changes the current light output to the maximum level
    RECALL_MIN_LEVEL = 0x06, // Changes the current light output to the minimum level
    STEP_DOWN_AND_OFF = 0x07, // If the target level is zero, lamp(s) are turned off; if the target level is between the min. and max. levels, decrease the target level by one; if the target level is max., lamp(s) are turned off
    ON_AND_STEP_UP = 0x08, // If the target level is zero, lamp(s) are set to minimum level; if target level is between min. and max. levels, increase the target level by one
    ENABLE_DAPC_SEQUENCE = 0x09, // Indicates the start of DAPC (level) commands
    GO_TO_LAST_ACTIVE_LEVEL = 0x0A, // Sets the target level to the last active output level
    CONTINUOUS_UP = 0x0B, // Fade started until max level reached
    CONTINUOUS_DOWN = 0x0C, // Fade started until min level reached
    GO_TO_SCENE = 0x10, // Sets a group of lamps to a predefined scene // Bits 0-3: Scene number
    DALI_RESET = 0x20, // Configures all variables back to their Reset state
    STORE_ACTUAL_LEVEL_IN_DTR0 = 0x21, // Stores the actual level value into Data Transfer Register 0 (DTR0)
    SAVE_PERSISTENT_VARIABLES = 0x22, // Stores all variables into Nonvolatile Memory (NVM)
    SET_OPERATING_MODE_DTR0 = 0x23, // Sets the operating mode to the value listed in DTR0
    RESET_MEMORY_BANK_DTR0 = 0x24, // Resets the memory bank identified by DTR0 (memory bank must be implemented and unlocked)
    IDENTIFY_DEVICE = 0x25, // Instructs a control gear to run an identification procedure
    SET_MAX_LEVEL_DTR0 = 0x2A, // Configures the control gear's maximum output level to the value stored in DTR0
    SET_MIN_LEVEL_DTR0 = 0x2B, // Configures the control gear's minimum output level to the value stored in DTR0
    SET_SYSTEM_FAILURE_LEVEL_DTR0 = 0x2C, // Sets the control gear's output level in the event of a system failure to the value stored in DTR0
    SET_POWER_ON_LEVEL_DTR0 = 0x2D, // Configures the output level upon power-up based on the value of DTR0
    SET_FADE_TIME_DTR0 = 0x2E, // Sets the fade time based on the value of DTR0
    SET_FADE_RATE_DTR0 = 0x2F, // Sets the fade rate based on the value of DTR0
    SET_SCENE = 0x40, // Configures scene 'x' based on the value of DTR0 // Bits 0-3: Scene number
    REMOVE_FROM_SCENE = 0x50, // Removes one of the control gears from a scene // Bits 0-3: Scene number
    ADD_TO_GROUP = 0x60, // Adds a control gear to a group // Bits 0-3: Group number
    REMOVE_FROM_GROUP = 0x70, // Removes a control gear from a group // Bits 0-3: Group number
    SET_SHORT_ADDRESS_DTR0 = 0x80, // Sets a control gear's short address to the value of DTR0
    ENABLE_WRITE_MEMORY = 0x81, // Allows writing into memory banks
    QUERY_STATUS = 0x90, // Determines the control gear's status based on a combination of gear properties
    QUERY_CONTROL_GEAR_PRESENT = 0x91, // Determines if a control gear is present
    QUERY_LAMP_FAILURE = 0x92, // Determines if a lamp has failed
    QUERY_LAMP_POWER_ON = 0x93, // Determines if a lamp is On
    QUERY_LIMIT_ERROR = 0x94, // Determines if the requested target level has been modified due to max. or min. level limitations
    QUERY_RESET_STATE = 0x95, // Determines if all NVM variables are in their Reset state
    QUERY_MISSING_SHORT_ADDRESS = 0x96, // Determines if a control gear's address is equal to 0xFF
    QUERY_VERSION_NUMBER = 0x97, // Returns the device's version number located in memory bank 0, location 0x16
    QUERY_CONTENT_DTR0 = 0x98, // Returns the value of DTR0
    QUERY_DEVICE_TYPE = 0x99, // Determines the device type supported by the control gear
    QUERY_PHYSICAL_MINIMUM = 0x9A, // Returns the minimum light output that the control gear can operate at
    QUERY_POWER_FAILURE = 0x9B, // Determines if an external power cycle occurred
    QUERY_CONTENT_DTR1 = 0x9C, // Returns the value of DTR1
    QUERY_CONTENT_DTR2 = 0x9D, // Returns the value of DTR2
    QUERY_OPERATING_MODE = 0x9E, // Determines the control gear's operating mode
    QUERY_LIGHT_SOURCE_TYPE = 0x9F, // Returns the control gear's type of light source
    QUERY_ACTUAL_LEVEL = 0xA0, // Returns the control gear's actual power output level
    QUERY_MAX_LEVEL = 0xA1, // Returns the control gear's maximum output setting
    QUERY_MIN_LEVEL = 0xA2, // Returns the control gear's minimum output setting
    QUERY_POWER_ON_LEVEL = 0xA3, // Returns the value of the intensity level upon power-up
    QUERY_SYSTEM_FAILURE_LEVEL = 0xA4, // Returns the value of the intensity level due to a system failure
    QUERY_FADE_TIME_FADE_RATE = 0xA5, // Returns a byte in which the upper nibble is equal to the fade time value and the lower nibble is the fade rate value
    QUERY_MANUFACTURER_SPECIFIC_MODE = 0xA6, // Returns a 'YES' when the operating mode is within the range of 0x80 - 0xFF
    QUERY_NEXT_DEVICE_TYPE = 0xA7, // Determines if the control gear has more than one feature, and if so, return the first/next device type or feature
    QUERY_EXTENDED_FADE_TIME = 0xA8, // Returns a byte in which bits 6-4 is the value of the extended fade time multiplier and the lower nibble is the extended fade time base
    QUERY_CONTROL_GEAR_FAILURE = 0xAA, // Determines if a control gear has failed
    QUERY_SCENE_LEVEL = 0xB0, // Returns the level value of scene 'x'
    QUERY_GROUPS_0_7 = 0xC0, // Returns a byte in which each bit represents a member of a group. A '1' represents a member of the group
    QUERY_GROUPS_8_15 = 0xC1, // Returns a byte in which each bit represents a member of a group. A '1' represents a member of the group
    QUERY_RANDOM_ADDRESS_H = 0xC2, // Returns the upper byte of a randomly generated address
    QUERY_RANDOM_ADDRESS_M = 0xC3, // Returns the high byte of a randomly generated address
    QUERY_RANDOM_ADDRESS_L = 0xC4, // Returns the low byte of a randomly generated address
    //READ_MEMORY_LOCATION = 0xC5 // Returns the content of the memory location stored in DTR0 that is located within the memory bank listed in DTR1
};

enum class DaliSpecialCommand : uint8_t {
    TERMINATE = 0xA1, // Stops the control gear's initialization
    DTR0_DATA = 0xA3, // Loads a data byte into DTR0
    INITIALISE = 0xA5, // Initializes a control gear, command must be issued twice
    RANDOMIZE = 0xA7, // Generates a random address value, command must be issued twice
    COMPARE = 0xA9, // Compares the random address variable to the search address variable
    WITHDRAW = 0xAB, // Changes the initialization state to reflect that a control gear had been identified but remains in the initialization state
    PING = 0xAD, // Used by control devices to indicate their presence on the bus
    SEARCH_ADDRH = 0xB1, // Determines if an address is present on the bus
    SEARCH_ADDRM = 0xB3, // Determines if an address is present on the bus
    SEARCH_ADDRL = 0xB5, // Determines if an address is present on the bus
    PROGRAM_SHORT_ADDRESS = 0xB7, // Programs a control gear's short address
    VERIFY_SHORT_ADDRESS = 0xB9, // Verifies if a control gear's short address is correct
    QUERY_SHORT_ADDRESS = 0xBB, // Queries a control gear's short address
    ENABLE_DEVICE_TYPE = 0xC1, // Enables a control gear's device type function
    DTR1_DATA = 0xC3, // Loads a data byte into DTR1
    DTR2_DATA = 0xC5, // Loads a data byte into DTR2
    WRITE_MEMORY_LOCATION = 0xC7, // Writes data into a specific memory location and returns the value of the data written
    WRITE_MEMORY_LOCATION_NO_REPLY = 0xC9 // Writes data into a specific memory location but does not return a response
};

// Extended device commands (DEVICE_LIGHT_TYPE_LED)
// Must be preceded by command SENABLE_DEVICE_TYPE
// https://github.com/sde1000/python-dali/blob/master/dali/gear/led.py
enum class DaliLedCommand : uint8_t {
    SELECT_DIMMING_CURVE = 0xE3, // Selects the dimming curve (1=linear, 0=logarithmic)
    STORE_DTR_AS_FAST_FADE_TIME = 0xE4,
    QUERY_GEAR_TYPE = 0xED,
    QUERY_DIMMING_CURVE = 0xEE, // See SELECT_DIMMING_CURVE
    QUERY_POSSIBLE_OPERATING_MODES = 0xEF, // Enum
    QUERY_FEATURES = 0xF0, // Enum
    QUERY_FAILURE_STATUS = 0xF1, // Enum
    QUERY_SHORT_CIRCUIT = 0xF2, // Boolean
    QUERY_OPEN_CIRCUIT = 0xF3, // Boolean
    QUERY_LOAD_DECREASE = 0xF4, // Boolean
    QUERY_LOAD_INCREASE = 0xF5, // Boolean
    QUERY_CURRENT_PROTECTOR_ACTIVE = 0xF6, // Boolean
    QUERY_THERMAL_SHUTDOWN = 0xF7, // Boolean
    QUERY_THERMAL_OVERLOAD = 0xF8, // Boolean
    QUERY_REFERENCE_MEASUREMENT_RUNNING = 0xF9, // Boolean
    QUERY_REFERENCE_MEASUREMENT_FAILED = 0xFA, // Boolean
    QUERY_CURRENT_PROTECTOR_ENABLED = 0xFB, // Boolean
    QUERY_OPERATING_MODE = 0xFC, // Enum
    QUERY_FAST_FADE_TIME = 0xFD,
    QUERY_MIN_FAST_FADE_TIME = 0xFE,
    QUERY_EXTENDED_VERSION_NUMBER = 0xFF
};

// Extended device commands (DEVICE_LIGHT_TYPE_COLOR)
// https://github.com/sde1000/python-dali/blob/master/dali/gear/colour.py
// Tc: Color Temperature in Mirek
// RGBWAF : R, G, B, White, Amber, 'Free Color'
// XY : XY chromaticity
enum class DaliColorCommand : uint8_t {
    SET_X_COORD = 224, // via DTR0 & DTR1
    SET_Y_COORD = 225, // via DTR0 & DTR1
    ACTIVATE = 226,
    SET_TEMPERATURE = 231, // Via DTR0 & DTR1
    TEMPERATURE_COOLER = 232,
    TEMPERATURE_WARMER = 233,
    SET_PRIMARY_NDIM_LEVEL = 234, // Via DTR0,1,2
    SET_RGB_DIM_LEVEL = 235, // Via DTR0,1,2,3
    SET_WAF_DIM_LEVEL = 236, // Via DTR0,1,2,3
    SET_RGBWAF_CONTROL = 237, // Via DTR0
    COPY_REPORT_TO_TEMPORARY = 238,
    STORE_TY_PRIMARY_N = 240, // Via DTR0,1,2,3
    STORE_XY_COORD_PRIMARY_N = 241, // Via DTR2
    ASSIGN_TO_LINKED_CHANNEL = 245, // Via DTR0
    START_AUTO_CALIBRATION = 246,
    QUERY_GEAR_FEATURES = 247,
    QUERY_COLOR_STATUS = 248,
    QUERY_COLOR_FEATURES = 249,
    QUERY_COLOR_VALUE = 250, // MSB in response, LSB in DTR0. Must call QueryActualLevel before calling this.
    QUERY_RGBWAF_CONTROL = 251,
    QUERY_ASSIGNED_COLOR = 252, // Depends on value of DTR0
    QUERY_EXTENDED_VERSION_NUMBER = 0xFF
};

enum class DaliColorFeature : uint8_t {
    XY_CAPABLE = 0x01,
    TC_CAPABLE = 0x02,
    // PrimaryNBit0 = 0x04,
    // PrimaryNBit1 = 0x08,
    // PrimaryNBit2 = 0x10,
    // RGBWAFChannelBit0 = 0x20,
    // RGBWAFChannelBit1 = 0x40,
    // RGBWAFChannelBit2 = 0x80,
};

// QUERY_LIGHT_SOURCE_TYPE
enum class DaliDeviceType : uint8_t {
    FLUORESCENT = 0,
    EMERGENCY = 1,
    HID = 2,
    LV_HALOGEN = 3,
    INCANDESCENT = 4,
    DIGITAL = 5,
    LED = 6,
    COLOR = 8
};

enum class DaliLedDimmingCurve : uint8_t {
    LOGARITHMIC = 0,
    LINEAR = 1
};

/// @brief Abstract class for interfacing with a physical DALI bus
class DaliPort {
public:
    virtual void sendForwardFrame(uint8_t address, uint8_t data) = 0;
    virtual uint8_t receiveBackwardFrame(unsigned long timeout_ms = 100);

public:
    virtual void resetBus() { }

    /// @brief Send a query command to the DALI bus and return the response
    /// @param address Device address, group address, or broadcast
    /// @param command Command byte
    /// @return Response byte (0xFF: success, 0x00: failure, or other byte)
    uint8_t sendQueryCommand(short_addr_t addr, DaliCommand command) {
        sendForwardFrame(
            (addr << 1) | DALI_COMMAND, 
            static_cast<uint8_t>(command));

        return receiveBackwardFrame();
    }

    /// @brief Send a control command to the DALI bus
    /// @param address Device address, group address, or broadcast
    /// @param command Command byte
    void sendControlCommand(short_addr_t addr, DaliCommand command) {
        // Control commands must send two back to back frames,
        // and will not return a response.
        sendForwardFrame(
            (addr << 1) | DALI_COMMAND, 
            static_cast<uint8_t>(command));

        sendForwardFrame(
            (addr << 1) | DALI_COMMAND, 
            static_cast<uint8_t>(command));
    }

    /// @brief Send a special command to the DALI bus
    /// @param special_command Special command affecting ALL devices on the bus
    /// @param data Parameter byte for the special command
    /// @return Most special commands do not send a response
    void sendSpecialCommand(DaliSpecialCommand command, uint8_t data) {
        sendForwardFrame(
            static_cast<uint8_t>(command), 
            static_cast<uint8_t>(data));
    }

    /// @brief Send an extended device command to the DALI bus
    /// @param short_address Device short address
    /// @param device_type See DEVICE_LIGHT_TYPE_* enum. Must not be 0
    /// @param extended_command Extended command specific to device type
    uint8_t sendExtendedQuery(short_addr_t addr, DaliDeviceType device_type, uint8_t extended_command) {
        sendSpecialCommand(
            DaliSpecialCommand::ENABLE_DEVICE_TYPE,
            static_cast<uint8_t>(device_type));

        sendForwardFrame(
            (addr << 1) | DALI_COMMAND, 
            static_cast<uint8_t>(extended_command));
        return receiveBackwardFrame();
    };

    uint8_t sendExtendedQuery(short_addr_t addr, DaliLedCommand led_command) {
        return sendExtendedQuery(addr, DaliDeviceType::LED, static_cast<uint8_t>(led_command));
    }
    uint8_t sendExtendedQuery(short_addr_t addr, DaliColorCommand color_command) {
        return sendExtendedQuery(addr, DaliDeviceType::COLOR, static_cast<uint8_t>(color_command));
    }

    /// @brief Send an extended device command to the DALI bus
    /// @param short_address Device short address
    /// @param device_type See DEVICE_LIGHT_TYPE_* enum. Must not be 0
    /// @param extended_command Extended command specific to device type
    void sendExtendedCommand(short_addr_t addr, DaliDeviceType device_type, uint8_t extended_command) {
        sendSpecialCommand(
            DaliSpecialCommand::ENABLE_DEVICE_TYPE,
            static_cast<uint8_t>(device_type));

        // MUST send twice, no response
        sendForwardFrame(
            (addr << 1) | DALI_COMMAND, 
            static_cast<uint8_t>(extended_command));

        sendForwardFrame(
            (addr << 1) | DALI_COMMAND, 
            static_cast<uint8_t>(extended_command));
    };

    void sendExtendedCommand(short_addr_t addr, DaliLedCommand led_command) {
        sendExtendedCommand(addr, DaliDeviceType::LED, static_cast<uint8_t>(led_command));
    }
    void sendExtendedCommand(short_addr_t addr, DaliColorCommand color_command) {
        sendExtendedCommand(addr, DaliDeviceType::COLOR, static_cast<uint8_t>(color_command));
    }

    /// @brief Set the DTR0 register
    /// @remarks This is how the protocol configures values, since we only have 2 bytes to work with (addr + cmd).
    /// This also affects ALL devices on the bus. (no address)
    /// @param data 8-bit value
    void setDtr0(uint8_t value) {
        sendSpecialCommand(DaliSpecialCommand::DTR0_DATA, value);
    }
    void setDtr1(uint8_t value) {
        sendSpecialCommand(DaliSpecialCommand::DTR1_DATA, value);
    }
    void setDtr2(uint8_t value) {
        sendSpecialCommand(DaliSpecialCommand::DTR2_DATA, value);
    }

    uint8_t getDtr0(short_addr_t addr) {
        return sendQueryCommand(addr, DaliCommand::QUERY_CONTENT_DTR0);
    }
};

/// @brief Bus manager for handling bus addresses
class DaliBusManager {
public:
DaliBusManager(DaliPort& port)
        : port(port)
    { }

    /// @brief Put a device into intialisation mode.
    /// @remark Required before you can send other special commands. Expires after 15 minutes, or when you send TERMINATE.
    /// @param addr Which devices to affect
    void initialize(uint8_t addr) {
        // addr:
        // 0000 0000 : All devices
        // 0AAA AAA1 : Only devices with this address
        // 1111 1111 : Only devices without a short address
        port.sendSpecialCommand(DaliSpecialCommand::INITIALISE, addr);
        port.sendSpecialCommand(DaliSpecialCommand::INITIALISE, addr);
    }

    /// @brief Tell all devices in initialize mode to randomize their addresses.
    void randomize() {
        port.sendSpecialCommand(DaliSpecialCommand::RANDOMIZE, 0);
        port.sendSpecialCommand(DaliSpecialCommand::RANDOMIZE, 0);
    }

    /// @brief Test if the new randomized address is <= the address programmed in SEARCH[H,M,L].
    bool compareSearchAddress(uint32_t search_address) {
        port.sendSpecialCommand(DaliSpecialCommand::SEARCH_ADDRH, (search_address >> 16) & 0xFF); // Set SEARCHH
        port.sendSpecialCommand(DaliSpecialCommand::SEARCH_ADDRM, (search_address >> 8) & 0xFF);  // Set SEARCHM
        port.sendSpecialCommand(DaliSpecialCommand::SEARCH_ADDRL, search_address & 0xFF);         // Set SEARCHL

        port.sendSpecialCommand(DaliSpecialCommand::COMPARE, 0);

        const unsigned long timeout_ms = 10;
        return (port.receiveBackwardFrame(timeout_ms) == 0xFF);
    }

    /// @brief Exit the initialization mode.
    void terminate() {
        port.sendSpecialCommand(DaliSpecialCommand::TERMINATE, 0);
        port.sendSpecialCommand(DaliSpecialCommand::TERMINATE, 0);
    }

    bool programShortAddress(uint8_t addr) {
        addr = ((addr & 0x3F) << 1) | DALI_COMMAND;
        port.sendSpecialCommand(DaliSpecialCommand::PROGRAM_SHORT_ADDRESS, addr);

        port.sendSpecialCommand(DaliSpecialCommand::VERIFY_SHORT_ADDRESS, addr);
        return (port.receiveBackwardFrame() == 0xFF);
    }

    void clearShortAddress() {
        port.sendSpecialCommand(DaliSpecialCommand::PROGRAM_SHORT_ADDRESS, 0x7F);
    }

    /// @brief Automatically assign sequential short addresses to all devices on the DALI bus
    /// @param assign ASSIGN_ALL, ASSIGN_UNINITIALIZED, or the short address for a specific device
    /// @return The number of devices found on the bus
    uint8_t autoAssignShortAddresses(uint8_t assign = ASSIGN_ALL, bool reset = true);

    //uint8_t scanAddresses(std::vector<uint32_t>& addresses);

    void startAddressScan();
    bool findNextAddress(short_addr_t& short_addr, uint32_t& long_addr);
    void withdrawCurrentDevice();
    void endAddressScan();

    bool isControlGearPresent(short_addr_t addr = ADDR_BROADCAST) {
        return port.sendQueryCommand(addr, DaliCommand::QUERY_CONTROL_GEAR_PRESENT) != 0;
    }

    bool isMissingShortAddress(short_addr_t addr = ADDR_BROADCAST) {
        return port.sendQueryCommand(addr, DaliCommand::QUERY_MISSING_SHORT_ADDRESS) != 0;
    }

    uint32_t queryAddress(short_addr_t short_addr) {
        uint32_t addr = 0;
        addr |= (uint32_t)port.sendQueryCommand(short_addr, DaliCommand::QUERY_RANDOM_ADDRESS_H) << 16;
        addr |= (uint32_t)port.sendQueryCommand(short_addr, DaliCommand::QUERY_RANDOM_ADDRESS_M) << 8;
        addr |= (uint32_t)port.sendQueryCommand(short_addr, DaliCommand::QUERY_RANDOM_ADDRESS_L);
        return addr;
    }

private:
    /// @brief Tell the device matching the address in SEARCH[H,M,L] to ignore COMPARE from now on.
    void withdraw(uint32_t address) {
        port.sendSpecialCommand(DaliSpecialCommand::SEARCH_ADDRH, (address >> 16) & 0xFF);
        port.sendSpecialCommand(DaliSpecialCommand::SEARCH_ADDRM, (address >> 8) & 0xFF);
        port.sendSpecialCommand(DaliSpecialCommand::SEARCH_ADDRL, address & 0xFF);
        port.sendSpecialCommand(DaliSpecialCommand::WITHDRAW, 0);
    }

    DaliPort& port;
    bool _is_scanning = false;
    uint32_t _current_addr = 0;
};

class DaliLamp {
public:
    DaliLamp(DaliPort& port)
        : port(port)
    { }

    /// @brief Set brightness via Direct Arc Power Control (DAPC)
    /// @remark P = 10^((level-1)/(253/3)) * P_100%/1000 (exponential curve)
    /// @param short_addr Device or group short address, or ADDR_BROADCAST
    /// @param level min..max. or, 0->min  255->stop fading
    void setBrightness(short_addr_t addr, uint8_t brightness) {
        //DALI_LOGD("DALI: Setting brightness to %d", brightness);
        // Serial.print("DALI: Brightness="); 
        // if (brightness == 0) Serial.println("MIN");
        // else if (brightness == 0xFF) Serial.println("STOP");
        // else Serial.println(brightness);
        
        port.sendForwardFrame((addr << 1), brightness);
    }

    /// @brief Turn off immediately without fading
    void turnOff(short_addr_t short_addr = ADDR_BROADCAST) {
        //DALI_LOGD("DALI: Lamp Off");
        //Serial.println("DALI: Lamp Off");
        port.sendControlCommand(short_addr, DaliCommand::OFF);
    }

    /// @brief Increase brightness with fade
    /// @param short_addr 
    void fadeUp(short_addr_t short_addr = ADDR_BROADCAST) {
        port.sendControlCommand(short_addr, DaliCommand::UP);
    }

    /// @brief Decrease brightness with fade, does not turn off when reaching minimum
    /// @param short_addr 
    void fadeDown(short_addr_t short_addr = ADDR_BROADCAST) {
        port.sendControlCommand(short_addr, DaliCommand::DOWN);
    }

    /// @brief Set brightness to maximum
    /// @param short_addr 
    void fadeToMaximum(short_addr_t short_addr = ADDR_BROADCAST) {
        //Serial.println("DALI: Brightness=MAX");
        //DALI_LOGD("DALI: Brightness=MAX");
        port.sendControlCommand(short_addr, DaliCommand::RECALL_MAX_LEVEL);
    }

    /// @brief Set brightness to minimum
    /// @param short_addr 
    void fadeToMinimum(short_addr_t short_addr = ADDR_BROADCAST) {
        //Serial.println("DALI: Brightness=MIN");
        //DALI_LOGD("DALI: Brightness=MIN");
        port.sendControlCommand(short_addr, DaliCommand::RECALL_MIN_LEVEL);
    }

    /// @brief Set the fade time
    /// @remark T = 1/2 * sqrt(2^fade_time) seconds
    /// @param short_addr Device short address
    /// @param fade_time 0..15 (0 -> disable fade)
    void setFadeTime(short_addr_t short_addr, uint8_t fade_time) {
        fade_time &= 0x0F;
        port.setDtr0(fade_time);
        port.sendControlCommand(short_addr, DaliCommand::SET_FADE_TIME_DTR0);
    }

    /// @brief Set the fade rate
    /// @remark F = 506 / sqrt(2^fade_rate) steps/second
    /// @param short_addr Device short address
    /// @param fade_rate 1..15
    void setFadeRate(short_addr_t short_addr, uint8_t fade_rate) {
        fade_rate &= 0x0F;
        port.setDtr0(fade_rate);
        port.sendControlCommand(short_addr, DaliCommand::SET_FADE_RATE_DTR0);

        // uint8_t fadetime = port.sendQueryCommand(short_addr, DaliCommand::QUERY_FADE_TIME_FADE_RATE);
        // if ((fadetime & 0x0F) != fade_rate) {
        //     Serial.println("ERROR: Failed to set Fade Rate");
        // }
    }

    /// @brief Set the power-on level
    /// @param short_addr Device short address
    /// @param power_on_level min..max, or 0
    void setPowerOnLevel(short_addr_t short_addr, uint8_t power_on_level) {
        port.setDtr0(power_on_level);
        if (port.getDtr0(short_addr) != power_on_level) {
            //Serial.println("WARNING: DTR0 not updated!");
            DALI_LOGE("WARNING: DTR0 not updated!");
            return;
        }

        port.sendControlCommand(short_addr, DaliCommand::SET_POWER_ON_LEVEL_DTR0);

        auto new_level = port.sendQueryCommand(short_addr, DaliCommand::QUERY_POWER_ON_LEVEL);
        if (new_level != power_on_level) {
            //Serial.println("WARNING: Power On Level not updated!");
            DALI_LOGE("WARNING: Power On Level not updated!");
            return;
        }
    }


    /// @brief Get the minimum allowable brightness level (usually 1)
    /// @param short_addr Device short address
    /// @return 1..254
    uint8_t getMinLevel(short_addr_t short_addr) {
        return port.sendQueryCommand(short_addr, DaliCommand::QUERY_MIN_LEVEL);
    }

    /// @brief Get the maximum allowable brightness level (usually 254)
    /// @param short_addr Device short address
    /// @return 1..254
    uint8_t getMaxLevel(short_addr_t short_addr) {
        return port.sendQueryCommand(short_addr, DaliCommand::QUERY_MAX_LEVEL);
    }

    /// @brief Get the configured default power-on level
    /// @param short_addr Device short address
    /// @return 1..254
    uint8_t getPowerOnLevel(short_addr_t short_addr) {
        return port.sendQueryCommand(short_addr, DaliCommand::QUERY_POWER_ON_LEVEL);
    }

    /// @brief Get the current brightness level
    /// @param short_addr Device short address
    /// @return 0..254
    uint8_t getCurrentLevel(short_addr_t short_addr) {
        return port.sendQueryCommand(short_addr, DaliCommand::QUERY_ACTUAL_LEVEL);
    }

    void setMinLevel(short_addr_t short_addr, uint8_t level) {
        port.setDtr0(level);
        if (port.getDtr0(short_addr) != level) {
            DALI_LOGE("WARNING: DTR0 not updated!");
            return;
        }
        port.sendControlCommand(short_addr, DaliCommand::SET_MIN_LEVEL_DTR0);
    }

    void setMaxLevel(short_addr_t short_addr, uint8_t level) {
        port.setDtr0(level);
        if (port.getDtr0(short_addr) != level) {
            DALI_LOGE("WARNING: DTR0 not updated!");
            return;
        }
        port.sendControlCommand(short_addr, DaliCommand::SET_MAX_LEVEL_DTR0);
    }

private:
    DaliPort& port;
};

class DaliLedClass {
public:
    DaliLedClass(DaliPort& port)
        : port(port)
    { }

    void setDimmingCurve(short_addr_t addr, DaliLedDimmingCurve curve) {
        port.setDtr0(static_cast<uint8_t>(curve));
        port.sendExtendedCommand(addr, DaliLedCommand::SELECT_DIMMING_CURVE);

        auto new_curve = static_cast<DaliLedDimmingCurve>(port.sendExtendedQuery(addr, DaliLedCommand::QUERY_DIMMING_CURVE));
        if (new_curve != curve) {
            //Serial.println("ERROR: Dimming Curve not updated!");
            DALI_LOGE("ERROR: Dimming Curve not updated!");
        }
    }

    DaliLedDimmingCurve getDimmingCurve(short_addr_t addr) {
        return static_cast<DaliLedDimmingCurve>(port.sendExtendedQuery(addr, DaliLedCommand::QUERY_DIMMING_CURVE));
    }

    /// @brief Set the fast fade time
    void setFastFadeTime(short_addr_t addr, uint8_t time) {
        port.setDtr0(time);
        port.sendExtendedCommand(addr, DaliLedCommand::STORE_DTR_AS_FAST_FADE_TIME);
    }

private:
    DaliPort& port;
};

enum class DaliColorParam : uint8_t {
    XCoordinate = 0,
    YCoordinate = 1,
    ColourTemperatureTC = 2,
    PrimaryNDimLevel0 = 3,
    PrimaryNDimLevel1 = 4,
    PrimaryNDimLevel2 = 5,
    PrimaryNDimLevel3 = 6,
    PrimaryNDimLevel4 = 7,
    PrimaryNDimLevel5 = 8,
    RedDimLevel = 9,
    GreenDimLevel = 10,
    BlueDimLevel = 11,
    WhiteDimLevel = 12,
    AmberDimLevel = 13,
    FreecolourDimLevel = 14,
    RGBWAFControl = 15,
    XCoordinatePrimaryN0 = 64,
    YCoordinatePrimaryN0 = 65,
    TYPrimaryN0 = 66,
    XCoordinatePrimaryN1 = 67,
    YCoordinatePrimaryN1 = 68,
    TYPrimaryN1 = 69,
    XCoordinatePrimaryN2 = 70,
    YCoordinatePrimaryN2 = 71,
    TYPrimaryN2 = 72,
    XCoordinatePrimaryN3 = 73,
    YCoordinatePrimaryN3 = 74,
    TYPrimaryN3 = 75,
    XCoordinatePrimaryN4 = 76,
    YCoordinatePrimaryN4 = 77,
    TYPrimaryN4 = 78,
    XCoordinatePrimaryN5 = 79,
    YCoordinatePrimaryN5 = 80,
    TYPrimaryN5 = 81,
    NumberOfPrimaries = 82,
    ColourTemperatureTcCoolest = 128,
    ColourTemperatureTcPhysicalCoolest = 129,
    ColourTemperatureTcWarmest = 130,
    ColourTemperatureTcPhysicalWarmest = 131,
    TemporaryXCoordinate = 192,
    TemporaryYCoordinate = 193,
    TemporaryColourTemperature = 194,
    TemporaryPrimaryNDimLevel0 = 195,
    TemporaryPrimaryNDimLevel1 = 196,
    TemporaryPrimaryNDimLevel2 = 197,
    TemporaryPrimaryNDimLevel3 = 198,
    TemporaryPrimaryNDimLevel4 = 199,
    TemporaryPrimaryNDimLevel5 = 200,
    TemporaryRedDimLevel = 201,
    TemporaryGreenDimLevel = 202,
    TemporaryBlueDimLevel = 203,
    TemporaryWhiteDimLevel = 204,
    TemporaryAmberDimLevel = 205,
    TemporaryFreecolourDimLevel = 206,
    TemporaryRgbwafControl = 207,
    TemporaryColourType = 208,
    ReportXCoordinate = 224,
    ReportYCoordinate = 225,
    ReportColourTemperatureTc = 226,
    ReportPrimaryNDimLevel0 = 227,
    ReportPrimaryNDimLevel1 = 228,
    ReportPrimaryNDimLevel2 = 229,
    ReportPrimaryNDimLevel3 = 230,
    ReportPrimaryNDimLevel4 = 231,
    ReportPrimaryNDimLevel5 = 232,
    ReportRedDimLevel = 233,
    ReportGreenDimLevel = 234,
    ReportBlueDimLevel = 235,
    ReportWhiteDimLevel = 236,
    ReportAmberDimLevel = 237,
    ReportFreecolourDimLevel = 238,
    ReportRgbwafControl = 239,
    ReportColourType = 240
};

#define STATUS_BALLAST_OK (0x01)
#define STATUS_LAMP_FAILURE (0x02)
#define STATUS_LAMP_ON (0x04)
#define STATUS_LIMIT_ERROR (0x08)
#define STATUS_FADE_STATE (0x10) // 0:ready, 1:fading
#define STATUS_RESET_STATE (0x20)
#define STATUS_MISSING_SHORT_ADDRESS (0x40)
#define STATUS_POWER_FAILURE (0x80)

// ECMD_COLOR_QUERY_FEATURES
#define COLOR_FEATURE_XY_CAPABLE (0x01)
#define COLOR_FEATURE_TC_CAPABLE (0x02)
// ...

// ECMD_COLOR_QUERY_STATUS
#define COLOR_STATUS_XY_OUT_RANGE (0x01)
#define COLOR_STATUS_TC_OUT_RANGE (0x02)
#define COLOR_STATUS_AUTO_CALIB_RUNNING (0x04)
#define COLOR_STATUS_AUTO_CALIB_SUCCESSFUL (0x08)
#define COLOR_STATUS_XY_ACTIVE (0x10)
#define COLOR_STATUS_TC_ACTIVE (0x20)
#define COLOR_STATUS_RGBWAF_ACTIVE (0x40)

/// @brief Warmest color temperature supported by spec
#define COLOR_MIREK_WARMEST (1000)
/// @brief Coolest color temperature supported by spec
#define COLOR_MIREK_COOLEST (10)

class DaliColorClass {
public:
    DaliColorClass(DaliPort& port)
        : port(port)
    { }

    bool supportsExtendedColor(short_addr_t short_addr) {
        return (port.sendExtendedQuery(short_addr, DaliColorCommand::QUERY_EXTENDED_VERSION_NUMBER) != 0);
    }

    /// @brief Supports color temperature
    /// @param short_addr 
    /// @return 
    bool isTcCapable(short_addr_t short_addr) {
        return (port.sendExtendedQuery(short_addr, DaliColorCommand::QUERY_COLOR_FEATURES) & (uint8_t)DaliColorFeature::TC_CAPABLE) != 0;
    }

    /// @brief Supports XY color coordinates
    /// @param short_addr 
    /// @return 
    bool isXYCapable(short_addr_t short_addr) {
        return (port.sendExtendedQuery(short_addr, DaliColorCommand::QUERY_COLOR_FEATURES) & (uint8_t)DaliColorFeature::XY_CAPABLE) != 0;
    }

    // TODO: RGB??

    /// @brief Set color temperature
    /// @param short_addr Device short address
    /// @param tc Temperature, in mireds
    void setColorTemperature(short_addr_t short_addr, uint16_t tc, bool start_fade = true) {
        //Serial.print("DALI: Tc="); Serial.println(tc);
        port.setDtr0(tc & 0xFF);
        port.setDtr1((tc >> 8) & 0xFF);
        port.sendExtendedCommand(short_addr, DaliColorCommand::SET_TEMPERATURE);

        if (start_fade) {
            port.sendExtendedCommand(short_addr, DaliColorCommand::ACTIVATE);
        }
    }

    /// @brief Warm color temperature
    /// @param short_addr Device short address
    void stepWarmer(short_addr_t short_addr = ADDR_BROADCAST) {
        port.sendExtendedQuery(short_addr, DaliColorCommand::TEMPERATURE_WARMER);
    }

    /// @brief Cool color temperature
    /// @param short_addr Device short address
    void stepCooler(short_addr_t short_addr = ADDR_BROADCAST) {
        port.sendExtendedQuery(short_addr, DaliColorCommand::TEMPERATURE_COOLER);
    }

    uint16_t queryParameter(short_addr_t short_addr, DaliColorParam query) {
        port.sendQueryCommand(short_addr, DaliCommand::QUERY_ACTUAL_LEVEL);
        port.setDtr0(static_cast<uint8_t>(query));
        uint8_t msb = port.sendExtendedQuery(short_addr, DaliColorCommand::QUERY_COLOR_VALUE);
        uint8_t lsb = port.getDtr0(short_addr);
        return (uint16_t)(msb << 8u) | (uint16_t)lsb;
    }

    /// @brief Query color temperature
    /// @param short_addr Device short address
    /// @param query Query type
    /// @return Color temperature in mireds
    uint16_t getColorTemperature(short_addr_t short_addr) {
        return queryParameter(short_addr, DaliColorParam::ReportColourTemperatureTc);
    }

private:
    DaliPort& port;
};

class DaliScene {
public:
    DaliScene(DaliPort& port)
        : port(port)
    { }

    /// @brief Add device to a group
    /// @remark Once added, a group can be addressed via (ADDR_GROUP | group_id) 
    /// @param short_addr Device short address
    /// @param group Group 0..15
    void addToGroup(short_addr_t short_addr, uint8_t group) {
        DaliCommand cmd = static_cast<DaliCommand>((uint8_t)DaliCommand::ADD_TO_GROUP | (group & 0x0F));
        port.sendControlCommand(short_addr, cmd);
    }

    /// @brief Remove a device from a group
    /// @param short_addr Device short address
    /// @param group Group 0..15
    void removeFromGroup(short_addr_t short_addr, uint8_t group) {
        DaliCommand cmd = static_cast<DaliCommand>((uint8_t)DaliCommand::REMOVE_FROM_GROUP | (group & 0x0F));
        port.sendControlCommand(short_addr, cmd);
    }

    /// @brief Activate a scene
    /// @remark Fade to the brightness level stored for the scene ID
    /// @param short_addr Device or group short address
    /// @param scene Scene ID 0..15
    void goToScene(short_addr_t short_addr, uint8_t scene) {
        DaliCommand cmd = static_cast<DaliCommand>((uint8_t)DaliCommand::GO_TO_SCENE | (scene & 0x0F));
        port.sendControlCommand(short_addr, cmd);
    }

    /// @brief Save the current level to the specified scene
    /// @param short_addr Device short address
    /// @param scene Scene ID 0..15
    void storeScene(short_addr_t short_addr, uint8_t scene) {
        port.sendControlCommand(short_addr, DaliCommand::STORE_ACTUAL_LEVEL_IN_DTR0);
        DaliCommand cmd = static_cast<DaliCommand>((uint8_t)DaliCommand::SET_SCENE | (scene & 0x0F));
        port.sendControlCommand(short_addr, cmd);
    }

    /// @brief Remove the scene
    /// @param short_addr Device short address
    /// @param scene Scene ID 0..15
    void removeScene(short_addr_t short_addr, uint8_t scene) {
        DaliCommand cmd = static_cast<DaliCommand>((uint8_t)DaliCommand::REMOVE_FROM_SCENE | (scene & 0x0F));
        port.sendControlCommand(short_addr, cmd);
    }


private:
    DaliPort& port;
};

/// @brief Dali Bus Master
class DaliMaster {
public:
    DaliMaster(DaliPort& port)
        : active_addr(ADDR_BROADCAST)
        , port(port)
        , bus_manager(port)
        , lamp(port)
        , led(port)
        , color(port)
        , scene(port)
    { }

public:
    // /// @brief Set brightness via Direct Arc Power Control (DAPC)
    // /// @remark P = 10^((level-1)/(253/3)) * P_100%/1000 (exponential curve)
    // /// @param short_addr Device or group short address, or ADDR_BROADCAST
    // /// @param level min..max. or, 0->min  255->stop fading
    // void setBrightness(short_addr_t addr, uint8_t brightness) {
    //     Serial.print("DALI: Brightness="); 
    //     if (brightness == 0) Serial.println("MIN");
    //     else if (brightness == 0xFF) Serial.println("STOP");
    //     else Serial.println(brightness);
        
    //     port.sendForwardFrame((addr << 1), brightness);
    // }

    void setActiveAddress(short_addr_t short_addr) {
        this->active_addr = short_addr;
    }

    bool isDevicePresent(short_addr_t short_addr) {
        return (port.sendQueryCommand(short_addr, DaliCommand::QUERY_CONTROL_GEAR_PRESENT) != 0);
    }

    void reset(short_addr_t short_addr) {
        port.sendControlCommand(short_addr, DaliCommand::DALI_RESET);
    }

    /// @brief Commit all settings to non-volatile memory
    /// @param short_addr Device short address
    void savePersistentVariables(short_addr_t short_addr) {
        port.sendControlCommand(short_addr, DaliCommand::SAVE_PERSISTENT_VARIABLES);
    }

    /// @brief Activate identification procedure (probably flashes the lamp?)
    /// @param short_addr Device short address
    void identifyDevice(short_addr_t short_addr) {
        port.sendControlCommand(short_addr, DaliCommand::IDENTIFY_DEVICE);
    }

    void dumpStatusForDevice(uint8_t addr);

public:
    short_addr_t active_addr;
    DaliPort& port;
    DaliBusManager bus_manager;
    DaliLamp lamp;
    DaliLedClass led;
    DaliColorClass color;
    DaliScene scene;
};
