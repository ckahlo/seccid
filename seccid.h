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
 
#ifndef _H_SECCID_
#define _H_SECCID_

#include <stdint.h>

//#define USB_VID 0x1209
//#define USB_PID 0xyyyy
#define USB_DEV 0x0100

uint32_t process(uint8_t*, uint32_t);

#endif
