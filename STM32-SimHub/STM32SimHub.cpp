#include "STM32SimHub.h"

char DEVICE_UNIQUE_ID[33]; // 32 hex chars + null

void GenerateDeviceUUID()
{
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();

    // Pure contiguous hex string (USB-safe, Windows-safe)
    snprintf(DEVICE_UNIQUE_ID,
             sizeof(DEVICE_UNIQUE_ID),
             "%08lX%08lX%08lX", //"%08lX-%08lX-%08lX", //"%08lX%08lX%08lX",
             uid0,
             uid1,
             uid2);
}
/*
String DEVICE_UNIQUE_ID;

//Generates the unique identifier for the device.
void GenerateDeviceUUID()
{
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();

    char uuid[37];

    snprintf(uuid, sizeof(uuid),
        "%08lX-%04lX-%04lX-%04lX-%08lX",
        uid0,
        (uid1 >> 16) & 0xFFFF,
        uid1 & 0xFFFF,
        (uid2 >> 16) & 0xFFFF,
        uid2);

    DEVICE_UNIQUE_ID = String(uuid);
}
*/
