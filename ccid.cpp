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

#include "Arduino.h"
#include "ccid.h"

#include "tusb.h"
#include "device/usbd.h"
#include "device/usbd_pvt.h"
#include "device/dcd.h"

#ifndef TINYUSB_API_VERSION
#define TINYUSB_API_VERSION 0
#endif

typedef struct {
	uint8_t itf_num;
	uint8_t ep_in;
	uint8_t ep_out;

	tu_fifo_t rx_ff;	// nothing is cleared on reset from here on
	tu_fifo_t tx_ff;
	uint8_t rx_ff_buf[CFG_TUD_CCID_RX_BUFSIZE];
	uint8_t tx_ff_buf[CFG_TUD_CCID_TX_BUFSIZE];

	CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_CCID_EP_BUFSIZE];
	CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_CCID_EP_BUFSIZE];
} ccidd_interface_t;

#define ITF_MEM_RESET_SIZE offsetof(ccidd_interface_t, rx_ff)

// TODO: multiple instances to be tested & completed
CFG_TUSB_MEM_SECTION static ccidd_interface_t _ccidd_itf[CFG_TUD_CCID];

//------------- Static member -------------//
uint8_t SECCID_USBD_CCID::_instance_count = 0;

uint8_t SECCID_USBD_CCID::getInstanceCount(void) {
	return _instance_count;
}

SECCID_USBD_CCID::SECCID_USBD_CCID(void) {
	_instance = INVALID_INSTANCE;
	setStringDescriptor("TinyUSB CCID");
}

uint16_t SECCID_USBD_CCID::getInterfaceDescriptor(uint8_t itfnum, uint8_t *buf, uint16_t bufsize) {
	_itf_num = itfnum;
	uint8_t desc[] = { TUD_CCID_DESCRIPTOR(itfnum, 0, CCID_EPOUT, CCID_EPIN, CFG_TUD_CCID_EP_BUFSIZE) };
	uint16_t const len = sizeof(desc);

	if (buf) {
		if (bufsize < len) {
			return 0;
		}
		memcpy(buf, desc, len);
	}
	return len;
}

bool SECCID_USBD_CCID::begin() {
	if (isValid()) { // already started
		return false;
	}

	if (!(_instance_count < CFG_TUD_CCID)) {
		return false;	// too many instances
	}

	if (!TinyUSBDevice.addInterface(*this))
		return false;

	_instance = _instance_count++;
	return true;
}

void SECCID_USBD_CCID::end(void) {
	// Reset configuration descriptor without CCID
	TinyUSBDevice.clearConfiguration();
	_instance_count = 0;
	_instance = INVALID_INSTANCE;
}

//--------------------------------------------------------------------+
// Read API
//--------------------------------------------------------------------+
static void _prep_out_transaction(ccidd_interface_t *p_itf) {
	uint8_t const rhport = 0;
	uint16_t available = tu_fifo_remaining(&p_itf->rx_ff);

	TU_VERIFY(available >= sizeof(p_itf->epout_buf),); // This pre-check reduces endpoint claiming
	TU_VERIFY(usbd_edpt_claim(rhport, p_itf->ep_out),); // claim endpoint

	available = tu_fifo_remaining(&p_itf->rx_ff); // fifo can be changed before endpoint is claimed
	if (available >= sizeof(p_itf->epout_buf)) {
		usbd_edpt_xfer(rhport, p_itf->ep_out, p_itf->epout_buf, sizeof(p_itf->epout_buf));
	} else {
		usbd_edpt_release(rhport, p_itf->ep_out); // Release endpoint since we don't make any transfer
	}
}

uint32_t tud_ccid_n_read(uint8_t itf, void *buffer, uint32_t bufsize) {
	ccidd_interface_t *p_itf = &_ccidd_itf[itf];
	TU_VERIFY(p_itf->ep_out);

	uint32_t const num_read = tu_fifo_read_n(&p_itf->rx_ff, buffer, bufsize);
	_prep_out_transaction(p_itf);
	return num_read;
}

//--------------------------------------------------------------------+
// Write API
//--------------------------------------------------------------------+
static uint32_t tud_ccid_write_n_flush(ccidd_interface_t *p_itf) {
	if (!tu_fifo_count(&p_itf->tx_ff)) // No data to send
		return 0;

	uint8_t const rhport = 0;
	TU_VERIFY(usbd_edpt_claim(rhport, p_itf->ep_in), 0); // skip if previous transfer not complete

	uint16_t count = tu_fifo_read_n(&p_itf->tx_ff, p_itf->epin_buf, sizeof(p_itf->epin_buf));
	if (count) {
		TU_ASSERT(usbd_edpt_xfer(rhport, p_itf->ep_in, p_itf->epin_buf, count), 0);
		return count;
	} else {
		usbd_edpt_release(rhport, p_itf->ep_in); // Release endpoint since we don't make any transfer
		return 0;
	}
}

uint32_t tud_ccid_n_write(uint8_t itf, void *buffer, uint32_t bufsize) {
	ccidd_interface_t *p_itf = &_ccidd_itf[itf];
	TU_VERIFY(p_itf->ep_in);

	uint16_t ret = tu_fifo_write_n(&p_itf->tx_ff, buffer, bufsize);
	return tud_ccid_write_n_flush(p_itf) > 0 ? ret : 0;
}

//--------------------------------------------------------------------+
// USBD Driver API
//--------------------------------------------------------------------+
static void ccid_init(void) {
	tu_memclr(&_ccidd_itf, sizeof(_ccidd_itf));

	for (uint8_t i = 0; i < CFG_TUD_CCID; i++) {
		ccidd_interface_t *p_itf = &_ccidd_itf[i];
		tu_fifo_config(&p_itf->rx_ff, p_itf->rx_ff_buf, CFG_TUD_CCID_RX_BUFSIZE, 1, false);
		tu_fifo_config(&p_itf->tx_ff, p_itf->tx_ff_buf, CFG_TUD_CCID_TX_BUFSIZE, 1, false);
	}
}

static void ccid_reset(uint8_t rhport) {
	(void) rhport;

	for (uint8_t i = 0; i < CFG_TUD_CCID; i++) {
		ccidd_interface_t *p_itf = &_ccidd_itf[i];
		tu_memclr(p_itf, ITF_MEM_RESET_SIZE);
		tu_fifo_clear(&p_itf->rx_ff);
		tu_fifo_clear(&p_itf->tx_ff);
	}
}

static uint16_t ccid_open(uint8_t rhport, tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
	if (desc_itf->bInterfaceClass != TUSB_CLASS_SMART_CARD)
		return 0; // not our interface class

	// desc_intf->bInterfaceSubClass == 0 && desc_intf->bInterfaceProtocol == 0
	uint16_t drv_len = sizeof(tusb_desc_interface_t);
	TU_VERIFY(max_len >= drv_len, 0);

	uint8_t const *p_desc = (uint8_t const*) desc_itf;

	//------------- CCID descriptor -------------//
	p_desc = tu_desc_next(p_desc);
	TU_ASSERT(CCID_DESC_TYPE_CCID == tu_desc_type(p_desc), 0);
	drv_len += tu_desc_len(p_desc);

	ccidd_interface_t *p_itf = NULL;
	for (uint8_t i = 0; i < CFG_TUD_CCID; i++) { // Find available interface
		if (_ccidd_itf[i].ep_in == 0 && _ccidd_itf[i].ep_out == 0) {
			p_itf = &_ccidd_itf[i];
			break;
		}
	}
	TU_ASSERT(p_itf);

	p_itf->itf_num = desc_itf->bInterfaceNumber;
	(void) p_itf->itf_num;

	//------------- Endpoint Descriptor -------------//
	p_desc = tu_desc_next(p_desc);
	uint8_t numEp = desc_itf->bNumEndpoints;
	TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, numEp, TUSB_XFER_BULK, &p_itf->ep_out, &p_itf->ep_in), 0);
	drv_len += numEp * sizeof(tusb_desc_endpoint_t);

	if (p_itf->ep_out) {
		_prep_out_transaction(p_itf);
	}

	if (p_itf->ep_in) {
		tud_ccid_write_n_flush(p_itf);
	}

	return drv_len;
}

static bool ccid_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
	return false; // no control transfers supported
}

static bool ccid_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
	(void) rhport;
	(void) result;

	uint8_t itf;
	ccidd_interface_t *p_itf;

	for (itf = 0; itf < CFG_TUD_CCID; itf++) { // Identify which interface to use
		p_itf = &_ccidd_itf[itf];
		if ((ep_addr == p_itf->ep_out) || (ep_addr == p_itf->ep_in))
			break;
	}
	TU_ASSERT(itf < CFG_TUD_CCID);

	if (ep_addr == p_itf->ep_out) { // receive new data
		tu_fifo_write_n(&p_itf->rx_ff, p_itf->epout_buf, (uint16_t) xferred_bytes);

		if (tud_ccid_rx_cb) // invoke receive callback if available
			tud_ccid_rx_cb(itf);

		_prep_out_transaction(p_itf); // prepare for next
	} else if (ep_addr == p_itf->ep_in) {
		if (tud_ccid_tx_cb)
			tud_ccid_tx_cb(itf, (uint16_t) xferred_bytes);

		tud_ccid_write_n_flush(p_itf);
	}

	return true;
}

// static void ccid_sof(uint8_t rhport, uint32_t frame_count) { }// optional

static usbd_class_driver_t const _ccid_driver = {
#if CFG_TUSB_DEBUG >= 2
		.name = "CCID", //
#endif
		.init = ccid_init, //
		.reset = ccid_reset, //
		.open = ccid_open, //
		.control_xfer_cb = ccid_control_xfer_cb, //
		.xfer_cb = ccid_xfer_cb, //
		.sof = NULL };

usbd_class_driver_t const* usbd_app_driver_get_cb(uint8_t *driver_count) { // callback to add application driver
	*driver_count = 1;
	return &_ccid_driver;
}

/**
 * convenience runner for CCID interface
 *
 */

uint8_t ccid_in[CCID_MSGLEN];

uint32_t SECCID_USBD_CCID::run(uint32_t (*cb)(uint8_t*, uint32_t)) {
	const uint8_t itf = _instance;
	const ccidd_interface_t *ccid = &_ccidd_itf[itf];

	uint8_t *rdBuf = ccid_in;
	uint32_t rdLen = tud_ccid_n_read(itf, rdBuf, sizeof(ccid_in));

	while (rdLen) {
		while (rdLen < CCID_HDR_SZ) {
			rdLen += tud_ccid_n_read(itf, &rdBuf[rdLen], sizeof(ccid_in) - rdLen);
		}

		ccid_msg_t *msg = (ccid_msg_t*) rdBuf;
		uint8_t *p = msg->data;
		rdLen -= (CCID_HDR_SZ + msg->length);
		rdBuf += (CCID_HDR_SZ + msg->length);

		uint32_t wrLen = 0;

		switch (msg->type) {
		case ICC_POWER_ON: {
			msg->type = DATA_BLOCK;
			msg->status = msg->error = msg->param = 0;  // status, error, clock
			p[wrLen++] = 0x3B; // maybe make this configurable
			p[wrLen++] = 0x80;
			p[wrLen++] = 0x01;
			p[wrLen++] = 0x81;
			break;
		}
		case ICC_POWER_OFF: // no operation
		case GET_SLOT_STATUS: {
			msg->type = SLOT_STATUS;
			msg->status = msg->error = msg->param = 0;  // clock
			break;
		}
		case XFR_BLOCK: {
			msg->type = DATA_BLOCK;

			int32_t res = cb ? cb(p, msg->length) : -1;
			if (res < 0) {
				msg->status = SLOT_STATUS_FAILED;
				msg->error = -res;
			} else {
				wrLen = res;
				msg->status = msg->error = msg->param = 0;
			}
			break;
		}
		case GET_PARAMETERS:
		case RESET_PARAMETERS:
		case SET_PARAMETERS: {
			msg->type = PARAMETERS;
			msg->length = msg->status = msg->error = 0;
			msg->param = 0x01; // protocol num
			break;
		}
		default:
			msg->type = SLOT_STATUS;
			msg->length = msg->error = msg->param = 0; // clock
			msg->status = SLOT_STATUS_FAILED; // status: failed
			break;

		}

		msg->length = wrLen;
		p = (uint8_t*) msg;
		wrLen += CCID_HDR_SZ;
		for (uint32_t n = 0; wrLen > 0; wrLen -= n, p += n) {
			n = tud_ccid_n_write(itf, p, wrLen);
			yield();
		}
	}

	return 0;
}

