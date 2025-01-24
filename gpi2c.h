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

/**
 * 
 */

#ifndef _H_GPI2C_
#define _H_GPSPI_

#include <stddef.h>
#include <stdint.h>

#include <Wire.h>

#define I2CImpl TwoWire

//namespace kisses { // keep it small & simple embedded security
namespace seccid { // secure element CCID

class GPI2C {
	I2CImpl *bus;
	uint8_t addr = 0x48, nad = 0x21, i2cmode = 0, maxTries = 255, atr[40], apduBuf[1000 + 16 + 8 + 10 + 5]; // payload + max padding + CMAC + T1 header (4 + 4 + 2) + response frame (superfluous)
	uint16_t ifsc = 254, apduCtr = 0;

	// I2C read / write
	uint32_t WRI2C(uint8_t *buf, uint32_t len);
	uint32_t RDI2C(uint8_t *buf, uint32_t len);
public:
	GPI2C(I2CImpl *bus, uint16_t addr = 0x48);

	void begin();
	void close();

	// I2C tranaction
	uint32_t I2CTX(uint8_t pcb, uint8_t *buf, uint32_t lc, uint32_t le);

	// T1 transaction
	uint32_t T1TX(uint8_t *buf, uint32_t lc, uint32_t le);
};

} // end namespace

#endif
