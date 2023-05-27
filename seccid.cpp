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

#include "seccid.h"
#include "gpi2c.h"
#include <Adafruit_NeoPixel.h>

#undef PIN_NEOPIXEL // override for QtPy RP2040 NeoPixel
#define PIN_NEOPIXEL   (12u)
#define NEOPIXEL_POWER (11u)

Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);

const uint8_t detectAID[] = { 0xD2, 0x76, 0x00, 0x00, 0x93, 0xFE, 0x00, 0x42 };

uint32_t callctr = 0;
TwoWire *seBus = &Wire;
uint8_t seAddr = 0x48;
seccid::GPI2C *se1;
uint32_t callSE(uint8_t *buf, uint32_t len);

void printHex(Stream &out, uint8_t *buf, uint32_t len) {
	for (uint32_t i = 0; i < len; out.printf("%2.2X", buf[i++]))
		;
}

uint32_t process(uint8_t *buf, uint32_t len) {
	const uint16_t CLAINS = ((buf[0] << 8) | buf[1]), P1P2 = ((buf[2] << 8) | buf[3]), LC =
			!buf[4] && len > 5 ? (buf[5] << 8) | buf[6] : buf[4];
	uint16_t SW1SW2 = 0x6D00;

	uint32_t x = 0, y = 0;

	Serial.printf("APDU: %4.4X %4.4X %4.4X %4.4X: ", len, CLAINS, P1P2, LC);
	printHex(Serial, buf, len);
	Serial.println();

	if (!callctr++) {
		pinMode(NEOPIXEL_POWER, OUTPUT); // NeoPixel Power
		digitalWrite(NEOPIXEL_POWER, 1);
		pixel.begin();
		pixel.fill(pixel.Color(31, 0, 0), 0, 1);
		pixel.show(); // indicate presence of power
		TwoWire &bus = *seBus;
		bus.begin();
		bus.setClock(1000000);
		bus.beginTransmission(seAddr);
		if (!bus.endTransmission()) {
			// init secure element
		}
	}

	if (CLAINS == 0x00A4 && P1P2 == 0x0400 && len > 5 && buf[4] == sizeof(detectAID) && memcmp(detectAID, &buf[4], sizeof(detectAID))) { // SELECT check for detection
		buf[y++] = 0x61;
		buf[y++] = 0x0A;
		buf[y++] = 0x4F;
		buf[y++] = 0x08;
		buf[y++] = 0xD2;
		buf[y++] = 0x76;
		buf[y++] = 0x00;
		buf[y++] = 0x00;
		buf[y++] = 0x93;
		buf[y++] = 0xFE;
		buf[y++] = 0x00;
		buf[y++] = 0x42;

		SW1SW2 = 0x9000;
	} else if (CLAINS == 0xFFFF) { // reserved class/instruction pair
		switch (P1P2 & 0xFF00) { // channel commands
		case 0xC000: { // get ping and current setting
			pixel.fill(pixel.Color(0, 255, 0), 0, 1);
			pixel.show();
			if (!seBus) {
				buf[y++] = 0xFF;
			} else if (seBus == &Wire) {
				buf[y++] = 0;
			} else if (seBus == &Wire1) {
				buf[y++] = 1;
			} else {
				buf[y++] = 0xFE;
			}

			buf[y++] = seAddr;

			TwoWire &bus = *seBus;
			bus.begin();
			bus.setClock(1000000);
			bus.beginTransmission(seAddr);
			if (!bus.endTransmission()) {
				// init secure element
				uint8_t apdu[261]; // 5 + 255 + 1
				uint32_t le = sizeof(apdu), n = 0;

				if (se1) {
					se1->close();
				}
				se1 = new seccid::GPI2C(&bus);
				se1->begin();


				n = se1->I2CTX(0xCF, apdu, 0, le); // soft reset
				Serial.printf("SE: %4.4X: ", n);
				printHex(Serial, apdu, n);
				Serial.println();

				// XXX: some Arduino stacks support only 32 byte I2C buffers
				//apdu[0] = 0x20; // 32 byte IFS
				apdu[0] = 0x80; // 128 byte IFS
				n = se1->I2CTX(0xC1, apdu, 1, 1);
				Serial.printf("SE: %4.4X\n", n);

				SW1SW2 = 0x9000;
			} else {
				SW1SW2 = 0x6A82;
			}
			break;
		}
		case 0xC100: { // scan I2C busses
			TwoWire &bus = !(P1P2 & 0x00FF) ? Wire : Wire1;
			bus.begin();
			bus.setClock(1000000);
			for (uint8_t addr = 0; addr < 0x80; ++addr) {
				bus.beginTransmission(addr);
				if (!bus.endTransmission()) {
					buf[y++] = addr;
				}
			}

			SW1SW2 = !y ? 0x6A82 : 0x9000;
			break;
		}
		case 0xC200: { // set I2C bus, maybe extend to GP-SPI
			seBus = !(P1P2 & 0x00FF) ? &Wire : &Wire1;
			SW1SW2 = 0x9000;
			break;
		}
		case 0xC300: { // set device address
			TwoWire &bus = *seBus;
			bus.begin();
			bus.setClock(1000000);
			bus.beginTransmission((P1P2 & 0x00FF));
			if (!bus.endTransmission()) {
				seAddr = (P1P2 & 0x00FF);
				SW1SW2 = 0x9000;
			} else {
				SW1SW2 = 0x6A82;
			}
			break;
		}
		default: // call SE otherweise
			return callSE(buf, len);
		}
	} else {
		return callSE(buf, len);
	}

	buf[y++] = SW1SW2 >> 8;
	buf[y++] = SW1SW2 & 0xFF;

	return y;
}

uint32_t callSE(uint8_t *buf, uint32_t len) {
	if (se1) {
		// XXX: handle extended length
		uint32_t lc = (len > 4) ? buf[4] : 0, le = (len > 4) ? buf[5 + lc] : 0; // last byte of command

		//uint32_t n = se1->T1TX(buf, len, le + 2); // add status word
		uint32_t n = se1->T1TX(buf, 5 + lc, le + 2); // add status word

		Serial.printf("> %4.4X, %4.4X, %4.4X: ", lc, le, n);
		printHex(Serial, buf, n);
		Serial.println();

		return n;
	} else {
		return -2;
	}
}
