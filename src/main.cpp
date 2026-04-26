#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "dali.h"

#define DALI_TX_PIN 15
#define DALI_RX_PIN 14


#if defined(ARDUINO)
/// @brief Bit-banged implementation of a DALI bus
class DaliSerialBitBangPort : public DaliPort {
public:
    DaliSerialBitBangPort(int txPin, int rxPin)
        : m_txPin(txPin), m_rxPin(rxPin)
    { }

protected:
    void sendForwardFrame(uint8_t address, uint8_t data) override;
    uint8_t receiveBackwardFrame(unsigned long timeout_ms = 100) override;

private:
    void writeBit(bool bit);
    void writeByte(uint8_t b);
    uint8_t readByte();

    int m_txPin;
    int m_rxPin;
};
#endif


DaliSerialBitBangPort dali_serial { DALI_TX_PIN, DALI_RX_PIN };
DaliMaster dali { dali_serial };

void setup() {
    pinMode(DALI_TX_PIN, OUTPUT);
    pinMode(DALI_RX_PIN, INPUT);
    digitalWrite(DALI_TX_PIN, LOW);
    Serial.begin(9600);

    delay(5000);

}

//uint8_t daliAutoAssignShortAddresses(uint8_t assign = ASSIGN_ALL, bool reset = true);

static bool has_assigned_address = false;

void loop() {
    static uint8_t address = 0; // Example address
    static uint8_t device_count = 0;

#if 1
    //Serial.print("DALI: Query control gear presence: ");
    if (!dali.bus_manager.isControlGearPresent()) {
        Serial.println("Control gear: not present");
        delay(5000);
        return;
    } else {
        Serial.println("Control gear: present");
    }

    //Serial.print("DALI: Query short addr: ");
    //if (dali.bus_manager.isMissingShortAddress()) {
    if (!has_assigned_address) {
        has_assigned_address = true;
        Serial.println("One or more devices are missing a short address");
        delay(15000);
        Serial.println("Assigning addresses...");

        device_count = dali.bus_manager.autoAssignShortAddresses(ASSIGN_ALL);
        //device_count = daliAutoAssignShortAddresses(ASSIGN_ALL, true);

        for (int addr = 0; addr < device_count; addr++) {
            dali.dumpStatusForDevice(addr);

            uint16_t tc_coolest = dali.color.queryParameter(addr, DaliColorParam::ColourTemperatureTcCoolest);
            uint16_t tc_warmest = dali.color.queryParameter(addr, DaliColorParam::ColourTemperatureTcWarmest);
            Serial.print("    Tc(c)="); Serial.println(tc_coolest);
            Serial.print("    Tc(w)="); Serial.println(tc_warmest);
        }
    } 

    
    // Simplified initialization procedure: (single device attached)
    // 1. SET DTR0 to short address
    // 2. Verify DTR0 contents
    // 3. STORE DTR AS SHORT ADDRESS (x2)
    // uint8_t short_addr = 0x01;
    // daliSendSpecialCommand(SCMD_DTR0_DATA, short_addr);
    // uint8_t verify_addr = daliSendSpecialCommand(SCMD_VERIFY_SHORT_ADDRESS, 0);
    // if (verify_addr == short_addr) {
    //     Serial.println("Short address verified");
    //     daliSendSpecialCommand(SCMD_PROGRAM_SHORT_ADDRESS, 0);
    //     daliSendSpecialCommand(SCMD_PROGRAM_SHORT_ADDRESS, 0);

    //     Serial.print("Short address: "); Serial.println(short_addr, HEX);
    // } else {
    //     Serial.println("Short address verification failed");

#endif

    uint8_t addr = 0;

    if (!dali.isDevicePresent(addr)) {
        Serial.println("DALI: Device not present!");
        return;
    }

    //dali.dumpStatusForDevice(0);

    //dali.reset(0);
    // dali.lamp.setFadeRate(0, 7);
    // dali.lamp.setFadeTime(0, 3);
    // dali.lamp.setPowerOnLevel(0, 10);
    // //dali.led.setFastFadeTime(0, 200);
    // dali.lamp.setMinLevel(0, 50);
    // Serial.print("New Min: "); Serial.println(dali.lamp.getMinLevel(0));
    // //dali.lamp.setBrightness(0, 0);
    // dali.led.setDimmingCurve(0, DaliLedDimmingCurve::Linear);


    dali.lamp.turnOff(ADDR_BROADCAST);
    delay(2000);

    dali.lamp.fadeToMaximum(ADDR_BROADCAST);
    delay(2000);

    // for (int i = 5; i > 0; i--) {
    //     daliSetBrightness(0, i*40);
    //     delay(2000);
    // }


    dali.color.setColorTemperature(ADDR_BROADCAST, COLOR_MIREK_COOLEST);
    Serial.print("Tc="); Serial.println(dali.color.getColorTemperature(addr), HEX);
    delay(2000);
    dali.color.setColorTemperature(ADDR_BROADCAST, COLOR_MIREK_WARMEST);
    Serial.print("Tc="); Serial.println(dali.color.getColorTemperature(addr), HEX);
    delay(2000);

    dali.lamp.fadeToMinimum(ADDR_BROADCAST);
    delay(2000);
}