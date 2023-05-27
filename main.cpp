/*
 * This file is part of the SECCID distribution (https://github.com/ckahlo/seccid).
 * Copyright (c) 2023 Christian Kahlo.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * DLR Exp.007
 * GlobalPlatform Secure Element CCID connector
 *
 */

#include "Arduino.h" // required for millis()
#include "ccid.h"
#include "seccid.h"

SECCID_USBD_CCID ccid;

tusb_desc_device_qualifier_t desc_qualifier = { 10, TUSB_DESC_DEVICE_QUALIFIER, 0x0200, 0, 0, 0, 64, 1, 0 };
uint8_t const* tud_descriptor_device_qualifier_cb() { // Adafruit lacks device qualifier descriptor
	return (uint8_t*) &desc_qualifier;
}
//uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index){}

extern "C" void setup() {
	TinyUSBDevice.setManufacturerDescriptor("DLR/CK");
	TinyUSBDevice.setProductDescriptor("Exp.007 SECCID");
	TinyUSBDevice.setID(USB_VID, USB_PID);
	TinyUSBDevice.setDeviceVersion(USB_DEV);

	ccid.begin();
	bool usbConnected = (*(uint32_t*) (0x50110000 + 0x50)) & (1 << 16);
	while (usbConnected && !Serial && millis() < 1000) {
	}
	Serial.println("DLR/CK Exp.007 SECCID booted.\n");
	Serial.flush();
}

extern "C" void loop() {
	if (Serial && Serial.available()) {
		String s = Serial.readString();
		Serial.printf("> %s\n", s.c_str());
	}

	ccid.run(process);
}
