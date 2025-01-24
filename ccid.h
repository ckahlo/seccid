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

#ifndef _H_CCID_
#define _H_CCID_

#include <Adafruit_TinyUSB.h>

#define CFG_TUD_CCID 			(1)

#define CFG_TUD_CCID_EP_BUFSIZE	(TUD_OPT_HIGH_SPEED ? 512 : 64)

#define CFG_TUD_CCID_RX_BUFSIZE	(256)
#define CFG_TUD_CCID_TX_BUFSIZE	(256)

#define CCID_HDR_SZ				(10) // CCID message header size
#define CCID_DESC_SZ			(54) // CCID function descriptor size
#define CCID_DESC_TYPE_CCID		(0x21) // CCID Descriptor

#define CCID_VERSION			(0x0110)
#define CCID_IFSD				(1024)
#define CCID_FEATURES			(0x40000 | 0x40 | 0x20 | 0x10 | 0x08 | 0x04 | 0x02)
#define CCID_MSGLEN				(CCID_IFSD + CCID_HDR_SZ)
#define CCID_CLAGET				(0xFF)
#define CCID_CLAENV				(0xFF)

// CCID Descriptor Template
// Interface number, string index, EP notification address and size, EP data address (out, in) and size.
#define TUD_CCID_DESCRIPTOR(_itfnum, _stridx, _epout, _epin, _epsize) \
  /* CCID Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, TUSB_CLASS_SMART_CARD, 0, 0, _stridx,\
  /* CCID Function, version, max slot index, supported voltages and protocols */\
  CCID_DESC_SZ, CCID_DESC_TYPE_CCID, U16_TO_U8S_LE(CCID_VERSION), 0, 0x7, U32_TO_U8S_LE(3),\
  /* default clock, maximum clock, num clocks, current datarate, max datarate */\
  U32_TO_U8S_LE(4000), U32_TO_U8S_LE(5000), 0, U32_TO_U8S_LE(9600), U32_TO_U8S_LE(625000),\
  /* num datarates, max IFSD, sync. protocols, mechanical, features */\
  0, U32_TO_U8S_LE(CCID_IFSD), U32_TO_U8S_LE(0), U32_TO_U8S_LE(0), U32_TO_U8S_LE(CCID_FEATURES),\
  /* max msg len, get response CLA, envelope CLA, LCD layout, PIN support, max busy slots */\
  U32_TO_U8S_LE(CCID_MSGLEN), CCID_CLAGET, CCID_CLAENV, U16_TO_U8S_LE(0), 0, 1,\
  \
  /* Endpoint Out */\
  7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize), 0,\
  /* Endpoint In */\
  7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize), 0\

// CCID reqests
#define ICC_POWER_ON		(0x62)
#define ICC_POWER_OFF		(0x63)
#define GET_SLOT_STATUS		(0x65)
#define XFR_BLOCK			(0x6F)
#define GET_PARAMETERS		(0x6C)
#define RESET_PARAMETERS	(0x6D)
#define SET_PARAMETERS		(0x61)

// CCID responses
#define DATA_BLOCK			(0x80)
#define SLOT_STATUS			(0x81)
#define PARAMETERS			(0x82)

// status values
#define SLOT_STATUS_OK		(0)
#define SLOT_STATUS_FAILED	(1 << 6)

// do not modify
#pragma scalar_storage_order little-endian
typedef struct {
	uint8_t type;
	uint32_t length;
	uint8_t slot;
	uint8_t seq;
	uint8_t status;
	uint8_t error;
	uint8_t param;
	uint8_t data[CCID_IFSD];
} __attribute__((packed)) ccid_msg_t;

/**
 * application driver for CCID decive class
 *
 */

class SECCID_USBD_CCID: public Adafruit_USBD_Interface {
public:
	virtual ~SECCID_USBD_CCID() {}
	SECCID_USBD_CCID(void);
	// from Adafruit_USBD_Interface
	uint16_t getInterfaceDescriptor(uint8_t itfnum, uint8_t *buf, uint16_t bufsize);
	bool begin();

	static uint8_t getInstanceCount(void);
	void end(void);

	typedef uint32_t (*apdu_callback_t)(uint8_t*, uint32_t);
	apdu_callback_t set_apdu_callback(apdu_callback_t);

	void process();

private:
	enum {
		INVALID_INSTANCE = 0xFF
	};
	static uint8_t _instance_count;

	uint8_t _instance = INVALID_INSTANCE;

	bool isValid(void) {
		return _instance != INVALID_INSTANCE;
	}

	apdu_callback_t cb;
};

#endif

//

TU_ATTR_WEAK void tud_ccid_rx_cb(uint8_t itf);
TU_ATTR_WEAK void tud_ccid_tx_cb(uint8_t itf, uint16_t xferred_bytes);
