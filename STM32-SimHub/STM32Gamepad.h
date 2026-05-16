#ifndef __STM32GAMEPAD_H__
#define __STM32GAMEPAD_H__

#include <Arduino.h>
#include "Adafruit_TinyUSB.h"

// Descriptor constants
#define STM32GAMEPAD_REPORT_ID 0x03
#define STM32GAMEPAD_BUTTON_COUNT 128   //TODO: Verify if 32 or 128 should be used. Suggested to use 128 due to button matrix indicies.
#define STM32GAMEPAD_AXIS_SAMPLING 10   //TODO: Try different sampling rates for performance testing
#define STM32GAMEPAD_AXIS_MINVALUE 0
#define STM32GAMEPAD_AXIS_MAXVALUE 1023 //TODO: Try using 4095 for 12-bit precision later to match STM32 ADC resolution.


// Actual packet sent to the PC.
struct __attribute__((packed)) STM32GamepadReport {
	uint8_t buttons[STM32GAMEPAD_BUTTON_COUNT / 8];
	uint16_t axis1;
	uint16_t axis2;
	uint16_t axis3;
	uint16_t axis4;
};

// STM32 SimHub custom HID descriptor:
// 128 buttons, then four 16-bit Generic Desktop axes (X, Y, Z, Rx).
static uint8_t const STM32GamepadReportDesc[] = {
	HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
	HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_REPORT_ID(STM32GAMEPAD_REPORT_ID)

		HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),
		HID_USAGE_MIN(1),
		HID_USAGE_MAX(STM32GAMEPAD_BUTTON_COUNT),
		HID_LOGICAL_MIN(0),
		HID_LOGICAL_MAX(1),
		HID_REPORT_SIZE(1),
		HID_REPORT_COUNT(STM32GAMEPAD_BUTTON_COUNT),
		HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),

		HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
		HID_USAGE(HID_USAGE_DESKTOP_X),
		HID_USAGE(HID_USAGE_DESKTOP_Y),
		HID_USAGE(HID_USAGE_DESKTOP_Z),
		HID_USAGE(HID_USAGE_DESKTOP_RX),
		HID_LOGICAL_MIN(STM32GAMEPAD_AXIS_MINVALUE),
		HID_LOGICAL_MAX_N(STM32GAMEPAD_AXIS_MAXVALUE, 2),
		HID_REPORT_SIZE(16),
		HID_REPORT_COUNT(4),
		HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),

	HID_COLLECTION_END
};

// TinyUSB gamepad wrapper. This replaces the old Joystick_ object.
class STM32Gamepad {
private:
	Adafruit_USBD_HID hid;
	STM32GamepadReport report;
	bool autoSendState = true;

public:
	void begin(bool initAutoSendState = true) {
		autoSendState = initAutoSendState;
		memset(&report, 0, sizeof(report));

		if (!TinyUSBDevice.isInitialized()) {
			TinyUSBDevice.begin(0);
		}

		hid.setReportDescriptor(STM32GamepadReportDesc, sizeof(STM32GamepadReportDesc));
		hid.setPollInterval(2);   //TODO: Adjust polling interval for better performance if needed.
		hid.begin();

		if (TinyUSBDevice.mounted()) {
			TinyUSBDevice.detach();
			delay(10);
			TinyUSBDevice.attach();
		}
	}

	void update() {
#ifdef TINYUSB_NEED_POLLING_TASK
		TinyUSBDevice.task();
#endif
	}

	void setButton(uint8_t button, uint8_t value) {
		if (button >= STM32GAMEPAD_BUTTON_COUNT) {
			return;
		}

		uint8_t byteIndex = button / 8;
		uint8_t bitMask = 1 << (button % 8);

		if (value) {
			report.buttons[byteIndex] |= bitMask;
		}
		else {
			report.buttons[byteIndex] &= ~bitMask;
		}

		if (autoSendState) {
			sendState();
		}
	}

	void setAxis(uint8_t axis, uint16_t value) {
		value = constrain(value, STM32GAMEPAD_AXIS_MINVALUE, STM32GAMEPAD_AXIS_MAXVALUE);

		switch (axis) {
		case 0:
			report.axis1 = value;
			break;
		case 1:
			report.axis2 = value;
			break;
		case 2:
			report.axis3 = value;
			break;
		case 3:
			report.axis4 = value;
			break;
		default:
			return;
		}

		if (autoSendState) {
			sendState();
		}
	}

	void setAxis1(uint16_t value) { setAxis(0, value); }
	void setAxis2(uint16_t value) { setAxis(1, value); }
	void setAxis3(uint16_t value) { setAxis(2, value); }
	void setAxis4(uint16_t value) { setAxis(3, value); }

	void sendState() {
		update();

		if (hid.ready()) {
			hid.sendReport(STM32GAMEPAD_REPORT_ID, &report, sizeof(report));
		}
	}
};


// One analog input mapped to one axis in the STM32Gamepad report.
class STM32GamepadAxis {
private:
	int lastAxisValue = -1;
	int axisPin = -1;
	int axisIdx = -1;
	int minimumInputValue = 0;
	int maximumInputValue = 1023;
	int samplingRate = STM32GAMEPAD_AXIS_SAMPLING;
	STM32Gamepad* joystick = NULL;
	float exponentialFactor = 1;

	float analogReadXXbit(uint8_t analogPin, uint8_t bits_of_precision) {
		uint8_t n = bits_of_precision > 10 ? bits_of_precision - 10 : 0;
		unsigned long oversample_num = 1UL << (2 * n);
		uint8_t divisor = 1 << n;
		unsigned long inner_sum = 0;

		for (unsigned long j = 0; j < oversample_num; j++) {
			inner_sum += analogRead(analogPin);
		}

		return (float)((inner_sum + (unsigned long)divisor / 2UL) >> n);
	}

	void setAxis(int axisIdx, int value) {
		if (joystick == NULL) {
			return;
		}

		joystick->setAxis(axisIdx, value);
		joystick->sendState();
	}

public:
	STM32GamepadAxis(byte axisPin, int axisIdx, int minimumInputValue, int maximumInputValue, int samplingRate, double exponentialFactor = 1) {
		this->axisIdx = axisIdx;
		this->axisPin = axisPin;
		this->minimumInputValue = minimumInputValue;
		this->maximumInputValue = maximumInputValue;
		this->samplingRate = samplingRate > 10 ? samplingRate : 10;
		this->exponentialFactor = exponentialFactor;
	}

	void SetJoystick(STM32Gamepad* joystick) {
		this->joystick = joystick;
		read();
	}

	bool read() {
		int pot = analogReadXXbit(axisPin, samplingRate);

		if (lastAxisValue == pot) {
			return false;
		}

		lastAxisValue = pot;

		int mapped = map(pot, minimumInputValue, maximumInputValue, STM32GAMEPAD_AXIS_MINVALUE, STM32GAMEPAD_AXIS_MAXVALUE);
		float mapped2 = min(1.0f, max(0.0f, (float)mapped / (float)STM32GAMEPAD_AXIS_MAXVALUE));

		if (exponentialFactor != 1) {
			mapped2 = pow(mapped2, 1.0f / exponentialFactor);
		}

		setAxis(axisIdx, (int)(mapped2 * STM32GAMEPAD_AXIS_MAXVALUE));
		return true;
	}
};

#endif
