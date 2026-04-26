#include "dali.h"
#include <Arduino.h>
#if defined(ARDUINO)

#define QUARTER_BIT_PERIOD 208
#define HALF_BIT_PERIOD 416
#define BIT_PERIOD 833

void DaliSerialBitBangPort::writeBit(bool bit) {
    // NOTE: output is inverted - HIGH will pull the bus to 0V (logic low)
    bit = !bit;
    digitalWrite(m_txPin, bit ? LOW : HIGH);
    delayMicroseconds(HALF_BIT_PERIOD-6);
    digitalWrite(m_txPin, bit ? HIGH : LOW);
    delayMicroseconds(HALF_BIT_PERIOD-6);
}

void DaliSerialBitBangPort::writeByte(uint8_t b) {
    for (int i = 0; i < 8; i++) {
        writeBit(b & 0x80);
        b <<= 1;
    }
}

uint8_t DaliSerialBitBangPort::readByte() {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte <<= 1;
        byte |= digitalRead(m_rxPin);
        delayMicroseconds(BIT_PERIOD); // 1/1200 seconds
    }
    return byte;
}

void DaliSerialBitBangPort::sendForwardFrame(uint8_t address, uint8_t data) {
    //Serial.print("TX: "); Serial.print(address, HEX); Serial.print(" "); Serial.println(data, HEX);

    writeBit(1); // START bit
    writeByte(address);
    writeByte(data);
    digitalWrite(m_txPin, LOW);
    delayMicroseconds(HALF_BIT_PERIOD*2);
    delayMicroseconds(BIT_PERIOD*4); // Optional, for clarity in scope trace
}

// void sendForwardFrameV2(uint8_t address, uint16_t data) {
//     //Serial.print("TX: "); Serial.print(address, HEX); Serial.print(" "); Serial.println(data, HEX);

//     daliWriteBit(1); // START bit
//     daliWriteByte(address);
//     daliWriteByte((data >> 8) & 0xFF);
//     daliWriteByte(data& 0xFF);
//     digitalWrite(m_txPin, LOW);
//     delayMicroseconds(416*6);
// }

uint8_t DaliSerialBitBangPort::receiveBackwardFrame(unsigned long timeout_ms) {
    unsigned long startTime = millis();
    // Wait for START bit
    while (digitalRead(m_rxPin) == LOW) {
        if (millis() - startTime >= timeout_ms) {
            //Serial.println("No reply");
            return 0;
        }
    }
    delayMicroseconds(BIT_PERIOD); // Wait for first data bit
    delayMicroseconds(QUARTER_BIT_PERIOD); // Wait a quater bit period to sample middle of first half bit
    uint8_t data = readByte();
    delayMicroseconds(BIT_PERIOD*2); // Wait for STOP bits
    //Serial.print("RX: "); Serial.println(data, HEX);

    delayMicroseconds(BIT_PERIOD*8); // Minimum time before we can send another forward frame
    return data;
}

#endif