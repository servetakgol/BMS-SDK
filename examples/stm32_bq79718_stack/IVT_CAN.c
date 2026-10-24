/*
 * IVT_CAN.c
 *
 *  IVT CAN akim sensoru surucusu.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "IVT_CAN.h"

#define IVT_OFFSET_I          0U
#define IVT_OFFSET_U1         1U
#define IVT_OFFSET_U2         2U
#define IVT_OFFSET_U3         3U
#define IVT_OFFSET_T          4U
#define IVT_OFFSET_W          5U
#define IVT_OFFSET_AS         6U
#define IVT_OFFSET_WH         7U

#define IVT_STATUS_ERROR_MASK 0xF0U   /* data[1] ust nibble: olcum hatasi bitleri (IVT dok.) */
#define IVT_FRAME_MIN_LEN     6U


typedef struct {
	IVT_CAN_Cfg_t			cfg;
	IVT_CAN_EventFnMap_t	eventFnMap;
	IVT_CAN_Data_t			data;
}_Handle_t;

static _Handle_t _hIvt;


void IVT_CAN_Init(const IVT_CAN_Cfg_t *cfg, const IVT_CAN_EventFnMap_t *eventFnMap) {

	memset(&_hIvt, 0, sizeof(_hIvt));

	_hIvt.cfg = *cfg;

	if (eventFnMap != NULL) {
		_hIvt.eventFnMap = *eventFnMap;
	}
}


const IVT_CAN_Data_t *IVT_CAN_GetData(void) {

	return &_hIvt.data;
}


static int32_t _readInt32Be(const uint8_t *data) {

	return (int32_t)(((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
	                 ((uint32_t)data[2] <<  8) |  (uint32_t)data[3]);
}


void IVT_CAN_OnFrame(uint32_t id, const uint8_t *data, uint8_t len, uint32_t tickMs) {

	if (len < IVT_FRAME_MIN_LEN || id < _hIvt.cfg.baseId || id > _hIvt.cfg.baseId + IVT_OFFSET_WH) {
		return;
	}

	if ((data[1] & IVT_STATUS_ERROR_MASK) != 0U) {
		return;   /* sensor "olcum gecersiz" diyor; haber verilmez, BMS stale ile yakalar */
	}

	int32_t  value    = _readInt32Be(&data[2]);
	uint32_t dataMask = 0U;

	switch (id - _hIvt.cfg.baseId) {

		case IVT_OFFSET_I:
			_hIvt.data.currentMa = _hIvt.cfg.invertCurrent ? -value : value;
			dataMask = IVT_CAN_DATA_CURRENT;
			break;

		case IVT_OFFSET_U1:
			_hIvt.data.u1Mv = value;
			dataMask = IVT_CAN_DATA_U1;
			break;

		case IVT_OFFSET_U2:
			_hIvt.data.u2Mv = value;
			dataMask = IVT_CAN_DATA_U2;
			break;

		case IVT_OFFSET_U3:
			_hIvt.data.u3Mv = value;
			dataMask = IVT_CAN_DATA_U3;
			break;

		case IVT_OFFSET_T:
			_hIvt.data.tempDeciDeg = (int16_t)value;      /* zaten 0.1 C */
			dataMask = IVT_CAN_DATA_TEMP;
			break;

		case IVT_OFFSET_AS:
			_hIvt.data.chargeMas = (int64_t)(_hIvt.cfg.invertCurrent ? -value : value) * 1000;
			dataMask = IVT_CAN_DATA_CHARGE;
			break;

		default:
			return;   /* W, Wh: bu ornekte kullanilmiyor */
	}

	if (_hIvt.eventFnMap.onDataUpdated != NULL) {
		_hIvt.eventFnMap.onDataUpdated(dataMask, tickMs);
	}
}
