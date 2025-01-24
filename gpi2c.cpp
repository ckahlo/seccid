/*
 * This file is part of the SECCID distribution (https://github.com/ckahlo/seccid).
 * Copyright (c) 2023 - 2025 Christian Kahlo.
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

#include "gpi2c.h"

//namespace kisses { // keep it small & simple embedded security
namespace seccid { // secure element CCID

const uint16_t crcTable[256] = { /**/
0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF, /**/
0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7, /**/
0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E, /**/
0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876, /**/
0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD, /**/
0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5, /**/
0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C, /**/
0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974, /**/
0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB, /**/
0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3, /**/
0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A, /**/
0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72, /**/
0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9, /**/
0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1, /**/
0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738, /**/
0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70, /**/
0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7, /**/
0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF, /**/
0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036, /**/
0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E, /**/
0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5, /**/
0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD, /**/
0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134, /**/
0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C, /**/
0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3, /**/
0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB, /**/
0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232, /**/
0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A, /**/
0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1, /**/
0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9, /**/
0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330, /**/
0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78, /**/
};

// CCITTCRC16
uint16_t CCITTCRC16(uint8_t *p, uint32_t len, uint16_t crc) {
	while (len--) {
		crc = crcTable[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	}
	return crc;
}

GPI2C::GPI2C(I2CImpl *bus, uint16_t addr) {
	this->bus = bus;
	this->addr = addr;
}

void GPI2C::begin() { // initial IFSC 249? arduino max is 255(uint8) -1 addr = 254 - 5 frame
	bus->setClock(1'000'000); // bus->setClock(3'400'000); // maximum

	//apdu[0] = 0x20;
	//se1.I2CTX(0xC1, &apdu[0], 1, 1)
	//I2CTX(xx, 0, 0, 0); // set IFSC, etc.
}

void GPI2C::close() { // currently noop
}

uint32_t GPI2C::WRI2C(uint8_t *buf, uint32_t len) {
	uint8_t i2cErr = -1;
	for (uint32_t i = 0, written = 0; i < maxTries && i2cErr != 0; i++) {
		bus->beginTransmission(addr);
		written = bus->write(buf, len);
		if ((i2cErr = bus->endTransmission(true)))
			delay(5);
	}
	return i2cErr;
}

uint32_t GPI2C::RDI2C(uint8_t *buf, uint32_t len) {
	uint32_t msgSz = 0;
	for (int i = 0; i < maxTries && msgSz == 0; i++) {
		if (!(msgSz = bus->requestFrom((uint8_t) addr, (uint8_t) len, (uint8_t) 1)))
			delay(5);
	}
	return msgSz <= 0 ? msgSz : bus->readBytes(buf, msgSz);
}

/* GlobalPlatform APDU Transport over SPI/I2C v1.0 | GPC_SPE_172 - also called "T=1'" */
uint32_t GPI2C::I2CTX(uint8_t pcb, uint8_t *buf, uint32_t lc, uint32_t le) {
	if (pcb == 0xCF)
		apduCtr = 0; // reset ADPU counter on ATR/CIP

	if (lc >= 4089) // XXX: check if buf == apduBuf[3] and re-use same buffer
		return -1; // XXX: use ATR/CIP IFSC to check for max frame size

	// TODO: check PCB byte for encoding details
	uint8_t t1frame[4 + (buf == NULL ? 0 : lc) + 2] = // if buf==NULL transmit no data, lc is info field
			{ nad, pcb, (uint8_t) ((buf == NULL) ? lc : (lc >> 8)), (uint8_t) ((buf == NULL) ? 0 : lc) };
	if (buf == NULL)
		lc = 0;
	memcpy(&t1frame[4], buf, lc);
	uint16_t crc = ~CCITTCRC16(&t1frame[0], 4 + lc, ~0);
	t1frame[4 + lc + 0] = crc >> 8;
	t1frame[4 + lc + 1] = crc;

	if (WRI2C(&t1frame[0], 4 + lc + 2) == 0) {
		le = RDI2C(&t1frame[0], 4);
		// TODO: CHECK [0] NAD = 0x12 & PCB chain/number
		pcb = t1frame[1], le = (t1frame[2] << 8) | t1frame[3];

		Serial.printf("I2TX-R: %2.2X, %2.2X %4.4X\n", t1frame[0], pcb, le);

//		if (!(pcb & 0x80)) { // ... data
//			if (buf != NULL)
//				le = RDI2C(&buf[0], le + 2); // +CRC? (+SW1SW2?)
//
//			// XXX: check CRC and request re-transmit if failed
//
//			if (pcb & 0x40) { // ... more data, acknowledge
//				t1frame[0] = nad;
//				t1frame[1] = (!(pcb & (1 << 6))) << 6;
//				t1frame[2] = t1frame[3] = 0;
//				uint16_t crc = ~CCITTCRC16(&t1frame[0], 4 + lc, ~0);
//				t1frame[4 + 0 + 0] = crc >> 8;
//				t1frame[4 + 0 + 1] = crc;
//				if (WRI2C(&t1frame[0], 4 + lc + 2) == 0) {
//					le = RDI2C(&t1frame[0], 4);
//					// TODO: CHECK [0] NAD = 0x12 & PCB chain/number
//					pcb = t1frame[1], le = (t1frame[2] << 8) | t1frame[3];
//
//					Serial.printf("I2TX-R2: %2.2X, %2.2X %4.4X", t1frame[0], pcb, le);
//
//					if (!(pcb & 0x80)) { // ... data
//						if (buf != NULL)
//							le = RDI2C(&buf[0], le + 2); // +CRC? (+SW1SW2?)
//					}
//
//				}
//			}
//		} else { // control code or error
//
//		}


		if (buf != NULL)
			le = RDI2C(&buf[0], le + 2); // +CRC? (+SW1SW2?)

		// XXX: if WTX, reply and read again
		// XXX: check CRC and request re-transmit if failed
		le -= 2;
		if (buf == NULL)
			le = 0;
	} else {
		le = 0;
		buf[le++] = 0x6F;
		buf[le++] = 0xFF;
	}

	return le;
}

uint32_t GPI2C::T1TX(uint8_t *buf, uint32_t li, uint32_t lo) {
	uint8_t chain = 0;
	uint32_t read = I2CTX((((apduCtr++) & 1) << 6) | (chain << 5), buf, li, lo);
	return read;
}

} // end namespace
