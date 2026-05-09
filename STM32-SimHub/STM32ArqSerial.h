#ifndef __STM32_ARQ_SERIAL_H__
#define __STM32_ARQ_SERIAL_H__

#include <Arduino.h>
#include "RingBuffer.h"

// =======================================================
// CRC-8 TABLE (same as original, AVR -> STM32 safe)
// =======================================================
static const uint8_t crc_table_crc8[256] = {
    0, 213, 127, 170, 254, 43, 129, 84, 41, 252, 86, 131, 215, 2, 168, 125,
    82, 135, 45, 248, 172, 121, 211, 6, 123, 174, 4, 209, 133, 80, 250, 47,
    164, 113, 219, 14, 90, 143, 37, 240, 141, 88, 242, 39, 115, 166, 12, 217,
    246, 35, 137, 92, 8, 221, 119, 162, 223, 10, 160, 117, 33, 244, 94, 139,
    157, 72, 226, 55, 99, 182, 28, 201, 180, 97, 203, 30, 74, 159, 53, 224,
    207, 26, 176, 101, 49, 228, 78, 155, 230, 51, 153, 76, 24, 205, 103, 178,
    57, 236, 70, 147, 199, 18, 184, 109, 16, 197, 111, 186, 238, 59, 145, 68,
    107, 190, 20, 193, 149, 64, 234, 63, 66, 151, 61, 232, 188, 105, 195, 22,
    239, 58, 144, 69, 17, 196, 110, 187, 198, 19, 185, 108, 56, 237, 71, 146,
    189, 104, 194, 23, 67, 150, 60, 233, 148, 65, 235, 62, 106, 191, 21, 192,
    75, 158, 52, 225, 181, 96, 202, 31, 98, 183, 29, 200, 156, 73, 227, 54,
    25, 204, 102, 179, 231, 50, 152, 77, 48, 229, 79, 154, 206, 27, 177, 100,
    114, 167, 13, 216, 140, 89, 243, 38, 91, 142, 36, 241, 165, 112, 218, 15,
    32, 245, 95, 138, 222, 11, 161, 116, 9, 220, 118, 163, 247, 34, 136, 93,
    214, 3, 169, 124, 40, 253, 87, 130, 255, 42, 128, 85, 1, 212, 126, 171,
    132, 81, 251, 46, 122, 175, 5, 208, 173, 120, 210, 7, 83, 134, 44, 249};

// =======================================================
// CRC MACRO (STM32-safe version)
// =======================================================
#define updateCrc(currentCrc, value) \
    (crc_table_crc8[(currentCrc) ^ (value)])

// =======================================================
// OPTIONAL CALLBACK
// =======================================================
typedef void (*IdleFunction)(bool);

// =======================================================
// ARQ SERIAL CLASS
// =======================================================
class STM32ARQSerial
{

private:
    static constexpr uint8_t MAX_PACKET_SIZE = 32;

    uint8_t partialBuffer[MAX_PACKET_SIZE];

    RingBuffer<uint8_t, 128> DataBuffer; // Increased buffer to support more LEDs!!

    IdleFunction idleFunction = nullptr;

    uint8_t lastValidPacket = 255;

    // ---------------- STATE MACHINE ----------------
    enum State
    {
        WAIT_H1,
        WAIT_H2,
        READ_ID,
        READ_LEN,
        READ_DATA,
        READ_CRC
    };

    State state = WAIT_H1;

    uint8_t packetID = 0;
    uint8_t length = 0;
    uint8_t index = 0;
    uint8_t crc = 0;
    uint8_t calcCrc = 0;

    uint32_t lastByteTime = 0;

    // =======================================================
    // RESET PACKET STATE
    // =======================================================
    void resetPacket()
    {
        state = WAIT_H1;
        index = 0;
        calcCrc = 0;
    }

    // =======================================================
    // SEND ACK / NACK
    // =======================================================
    void sendAck(uint8_t id)
    {
        Serial.write(0x03);
        Serial.write(id);
    }

    void sendNAck(uint8_t last, uint8_t reason)
    {
        Serial.write(0x04);
        Serial.write(last);
        Serial.write(reason);
    }

    // =======================================================
    // BYTE PROCESSOR (CORE PARSER)
    // =======================================================
    void processByte(uint8_t c)
    {

        lastByteTime = millis();

        switch (state)
        {

        case WAIT_H1:
            if (c == 0x01)
                state = WAIT_H2;
            break;

        case WAIT_H2:
            state = (c == 0x01) ? READ_ID : WAIT_H1;
            break;

        case READ_ID:
            packetID = c;
            calcCrc = updateCrc(0, packetID);
            state = READ_LEN;
            break;

        case READ_LEN:
            length = c;

            if (length == 0 || length > MAX_PACKET_SIZE)
            {
                resetPacket();
                break;
            }

            calcCrc = updateCrc(calcCrc, length);
            index = 0;
            state = READ_DATA;
            break;

        case READ_DATA:
            partialBuffer[index++] = c;
            calcCrc = updateCrc(calcCrc, c);

            if (index >= length)
            {
                state = READ_CRC;
            }
            break;

        case READ_CRC:
            crc = c;

            if (crc == calcCrc)
            {

                uint8_t next = (lastValidPacket > 127) ? 0 : lastValidPacket + 1;

                if (packetID == next || packetID == 255)
                {

                    for (uint8_t i = 0; i < length; i++)
                    {
                        DataBuffer.push(partialBuffer[i]);
                    }

                    lastValidPacket = packetID;
                }

                sendAck(packetID);
            }
            else
            {
                sendNAck(lastValidPacket, 0x04);
            }

            resetPacket();
            break;
        }
    }

public:
    // =======================================================
    // INIT CALLBACK
    // =======================================================
    void setIdleFunction(IdleFunction fn)
    {
        idleFunction = fn;
    }

    // =======================================================
    // MUST CALL IN LOOP FREQUENTLY
    // =======================================================
    void update()
    {

        if (idleFunction)
            idleFunction(false);

        while (Serial.available())
        {
            processByte((uint8_t)Serial.read());
        }

        // timeout safety reset
        if (state != WAIT_H1 && (millis() - lastByteTime > 50))
        {
            resetPacket();
        }
    }

    // =======================================================
    // READ API (NON-BLOCKING)
    // =======================================================
    int read()
    {
        uint32_t start = millis();

        do
        {
            update();

            if (DataBuffer.size() > 0)
            {
                uint8_t v;
                DataBuffer.pop(v);
                return v;
            }

            if (idleFunction)
                idleFunction(false);
        } while (millis() - start < 400);

        return -1;
    }



    int Available()
    {
        update();
        return DataBuffer.size();
    }

    // =======================================================
    // WRITE API (NO FLUSH - IMPORTANT FOR STM32)
    // =======================================================
    void write(uint8_t data)
    {
        Serial.write(0x08);
        Serial.write(data);
    }

    void CustomPacketStart(uint8_t type, uint8_t len)
    {
        Serial.write(0x09);
        Serial.write(type);
        Serial.write(len);
    }

    void CustomPacketSendByte(uint8_t data)
    {
        Serial.write(data);
    }

    void CustomPacketEnd()
    {
        // intentionally empty
    }

    // =======================================================
    // PRINT HELPERS
    // =======================================================
    void print(const char *str)
    {
        Serial.write(0x06);
        Serial.write(strlen(str));
        Serial.write((const uint8_t *)str, strlen(str));
        Serial.write(0x20);
    }

    void print(char c)
    {
        write((uint8_t)c);
    }
};

#endif