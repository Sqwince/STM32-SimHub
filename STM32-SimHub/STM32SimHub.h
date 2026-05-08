#pragma once
#include <Arduino.h>

extern char DEVICE_UNIQUE_ID[33];

void GenerateDeviceUUID();


// Fake an Arduino Micro (ATmega32U4)
#define SIGNATURE_0 0x1E
#define SIGNATURE_1 0x95
#define SIGNATURE_2 0x87
//STM32 Boards.txt updated to use VID/PID below
//VID: 0x2341 Arduino
//PID: 0x8037 Micro


// Fake an Arduino Mega (ATmega2560)
//#define SIGNATURE_0 0x1E
//#define SIGNATURE_1 0x98
//#define SIGNATURE_2 0x01


//#define SIGNATURE_0 ((HAL_GetUIDw0() >> 0)  & 0xFF)
//#define SIGNATURE_1 ((HAL_GetUIDw0() >> 8)  & 0xFF)
//#define SIGNATURE_2 ((HAL_GetUIDw0() >> 16) & 0xFF)



// A unique identifier for the device.
// in the future we could use the bytes to generate some
// other format (ala UUID), but now it's just a unique
// string tied to the device.
//String getUniqueId();