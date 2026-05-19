#ifndef __SHFASTIO_H__
#define __SHFASTIO_H__

#include <Arduino.h>

class FastDigitalPin {
private:
    uint32_t bit = 0;              // 32-bit mask for STM32
    GPIO_TypeDef *port = nullptr;  // STM32 GPIO port pointer
    bool pinIsValid = false;

public:

    bool isValid() {
        return pinIsValid;
    }

    void begin(int pin) {
        if (pin < 0) {
            pinIsValid = false;
            port = nullptr;
            bit = 0;
        }
        else {
            port = digitalPinToPort(pin);
            bit  = digitalPinToBitMask(pin);

            if (port == nullptr) {
                pinIsValid = false;
            } else {
                pinIsValid = true;
            }
        }
    }

    int digitalRead() {
        if (!pinIsValid || port == nullptr) return LOW;

        // Direct STM32 register read
        return (port->IDR & bit) ? HIGH : LOW;
    }
};

#endif