/*
 * BQ_AFE_Connector.c
 *
 *  BQ_AFE surucusu <-> BMS monitor connector'i.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "BQ_AFE_Connector.h"

#define BQ_AFE_CONN_TEMP_GROUP_NTC   0U
#define BQ_AFE_CONN_TEMP_GROUP_DIE   1U


static BQ_AFE_CONN_Cfg_t	_cfg;

/* BMS'e baglanan cihaz durumu: faultSummary'den cevrilir, BMS yerinde okur. */
static BMS_DeviceStatus_t	_deviceStatus[BQ_AFE_MAX_DEVICES];

/* BMS hook'larinin biriktirdigi istekler; Update'te surucuye aktarilir. */
static uint32_t				_appliedMask[BQ_AFE_MAX_DEVICES];
static uint32_t				_pendingMask[BQ_AFE_MAX_DEVICES];
static uint32_t				_pendingMaskDevices;        /* bit = deviceIndex (<= 32 cihaz) */
static bool					_measurementPending;
static bool					_clearFaultsPending;


/* ====================================================================== */
/*  BMS hook'lari (BMS_Run icinden cagrilir; bloklamaz, sadece biriktirir) */
/* ====================================================================== */

static void _applyBalanceMask(uint8_t moduleIndex, uint32_t cellMask, void *ctx) {

	(void)ctx;

	if (moduleIndex >= _cfg.numOfDevices) {
		return;
	}

	_pendingMask[moduleIndex] = cellMask;
	_pendingMaskDevices |= (1UL << moduleIndex);
}

static void _requestMeasurement(uint8_t moduleIndex, void *ctx) {

	(void)moduleIndex; (void)ctx;
	_measurementPending = true;         /* stack read; cihaz bazli ayrim yok */
}

static void _clearDeviceFaults(uint8_t moduleIndex, void *ctx) {

	(void)moduleIndex; (void)ctx;
	_clearFaultsPending = true;
}


/* ====================================================================== */
/*  Init: her cihaz icin monitor connector kur                             */
/* ====================================================================== */

bool BQ_AFE_CONN_Init(const BQ_AFE_CONN_Cfg_t *cfg) {

	if (cfg == NULL || cfg->numOfDevices == 0U || cfg->numOfDevices > BQ_AFE_MAX_DEVICES) {
		return false;
	}

	_cfg = *cfg;
	memset(_deviceStatus, 0, sizeof(_deviceStatus));
	memset(_appliedMask,  0, sizeof(_appliedMask));
	memset(_pendingMask,  0, sizeof(_pendingMask));
	_pendingMaskDevices = 0U;
	_measurementPending = false;
	_clearFaultsPending = false;

	for (uint8_t deviceIndex = 0; deviceIndex < cfg->numOfDevices; deviceIndex++) {

		const BQ_AFE_Data_t *data = BQ_AFE_GetData(deviceIndex);

		/* Gecici struct: BMS baglari kendi tablosuna kopyalar. */
		BMS_MonitorConnector_t connector = {
			.cellVoltagesMv		= data->cellVoltagesMv,          /* [0] = hucre 1 (D5 normalize) */
			.numOfCells			= cfg->numOfCellsPerDevice,
			.moduleVoltageMv	= &data->batVoltageMv,           /* BAT pini; BMS hucre toplami ile karsilastirir */
			.deviceStatus		= &_deviceStatus[deviceIndex],
			.numOfTempGroups	= 2,
			.tempGroups = {
				[BQ_AFE_CONN_TEMP_GROUP_NTC] = {
					.values		= &data->gpioVoltagesMv[cfg->ntcGpioFirst],   /* ratiometric mV */
					.firstSlot	= (uint16_t)(cfg->ntcSlotBase + deviceIndex * cfg->numOfNtcPerDevice),
					.numOfSlots	= cfg->numOfNtcPerDevice,
					.role		= BMS_TEMP_ROLE_CELL,
					.ntc		= cfg->ntc,                                   /* BMS deciDeg'e cevirir */
					.refMv		= cfg->tsRefMv,
				},
				[BQ_AFE_CONN_TEMP_GROUP_DIE] = {
					.values		= data->dieTempDeciDeg,                       /* zaten deciDeg */
					.firstSlot	= (uint16_t)(cfg->dieSlotBase + deviceIndex * 2U),
					.numOfSlots	= 2,
					.role		= BMS_TEMP_ROLE_AFE_DIE,
					.ntc		= NULL,
					.refMv		= 0,
				},
			},
			.applyBalanceMask	= _applyBalanceMask,
			.applyFetEnable		= NULL,                          /* BQ79718'de entegre FET yok */
			.requestMeasurement	= _requestMeasurement,
			.clearDeviceFaults	= _clearDeviceFaults,
			.ctx				= NULL,
		};

		if (BMS_AttachMonitorConnector(cfg->packIndex, deviceIndex, &connector) != BMS_STATUS_OK) {
			return false;
		}
	}

	return true;
}


/* ====================================================================== */
/*  Surucu olaylari -> BMS_NotifyMonitor                                   */
/* ====================================================================== */

static void _translateDeviceStatus(uint8_t deviceIndex, const BQ_AFE_Data_t *data) {

	const BQ_AFE_FaultSummary_t *summary = &data->faultSummary;
	BMS_DeviceStatus_t *status = &_deviceStatus[deviceIndex];

	/* FAULT_SUMMARY OV ve UV'yi ayirmaz; ayrim gerekirse FAULT_OV/UV grubu
	 * okunup burada ayristirilir. Simdilik ikisi birden isaretlenir. */
	status->faultFlags.raw = 0U;
	status->faultFlags.mapped.overVolt    = summary->faultOvuv;
	status->faultFlags.mapped.underVolt   = summary->faultOvuv;
	status->faultFlags.mapped.overTemp    = summary->faultOtut;
	status->faultFlags.mapped.underTemp   = summary->faultOtut;
	status->faultFlags.mapped.overCurrent = summary->faultProt;
	status->faultFlags.mapped.comm        = summary->faultComm;
	status->faultFlags.mapped.system      = summary->faultSys;
	status->faultFlags.mapped.power       = summary->faultPwr;
	status->faultFlags.mapped.otp         = summary->faultOtp;
	status->faultFlags.mapped.adc         = summary->faultCompAdc;

	status->commErrorCount = data->commErrorCount;
	status->online         = data->online;
}


void BQ_AFE_CONN_OnDataUpdated(uint8_t deviceIndex, BQ_AFE_DataGroup_t group, const BQ_AFE_Data_t *data, uint32_t tickMs) {

	if (deviceIndex >= _cfg.numOfDevices) {
		return;
	}

	switch (group) {

		case BQ_AFE_DATA_GROUP_CELL_VOLTAGES: {
			/* Dengeleme acikken alinan ornek: BMS filtreler. Hangi cihazda acik
			 * oldugunu bu connector bilir (_appliedMask). */
			uint32_t flags = (_appliedMask[deviceIndex] != 0U) ? BMS_FEED_FLAG_BALANCING_ACTIVE : 0U;
			(void)BMS_NotifyMonitor(_cfg.packIndex, deviceIndex, BMS_MONITOR_DATA_CELLS, tickMs, flags);
			break;
		}

		case BQ_AFE_DATA_GROUP_GPIO_VOLTAGES:
			(void)BMS_NotifyMonitor(_cfg.packIndex, deviceIndex, BMS_MONITOR_DATA_TEMP_GROUP(BQ_AFE_CONN_TEMP_GROUP_NTC), tickMs, 0U);
			break;

		case BQ_AFE_DATA_GROUP_DIE_TEMP:
			(void)BMS_NotifyMonitor(_cfg.packIndex, deviceIndex, BMS_MONITOR_DATA_TEMP_GROUP(BQ_AFE_CONN_TEMP_GROUP_DIE), tickMs, 0U);
			break;

		case BQ_AFE_DATA_GROUP_PACK_VOLTAGES:
			(void)BMS_NotifyMonitor(_cfg.packIndex, deviceIndex, BMS_MONITOR_DATA_MODULE_VOLT, tickMs, 0U);
			break;

		case BQ_AFE_DATA_GROUP_FAULT_SUMMARY:
		case BQ_AFE_DATA_GROUP_DEV_STATUS:
			_translateDeviceStatus(deviceIndex, data);     /* 1 byte ceviri, kopya onemsiz */
			(void)BMS_NotifyMonitor(_cfg.packIndex, deviceIndex, BMS_MONITOR_DATA_STATUS, tickMs, 0U);
			break;

		default:
			break;   /* ID vb. BMS'i ilgilendirmez */
	}
}


void BQ_AFE_CONN_OnCommError(uint8_t deviceIndex, uint32_t tickMs) {

	if (deviceIndex >= _cfg.numOfDevices) {
		return;
	}

	/* Surucu veri uretemedi: comm bayragi + offline. Stale sayaclari zaten
	 * isler; bu sadece nedeni netlestirir. */
	_deviceStatus[deviceIndex].faultFlags.mapped.comm = 1U;
	_deviceStatus[deviceIndex].online = false;
	_deviceStatus[deviceIndex].commErrorCount++;

	(void)BMS_NotifyMonitor(_cfg.packIndex, deviceIndex, BMS_MONITOR_DATA_STATUS, tickMs, 0U);
}


/* ====================================================================== */
/*  Update: biriken istekleri surucuye aktar (surucu mesgulse bir sonraki tur) */
/* ====================================================================== */

void BQ_AFE_CONN_Update(void) {

	/* BQ79718: hucre basina CB sure sayaci. Mask'teki hucrelere sure kodu
	 * yazilir, digerleri 0. Sure dolunca cip kendi kapatir; BMS duty-cycle'i
	 * mask'i periyodik yeniledigi icin dead-man gibi calisir. */
	for (uint8_t deviceIndex = 0; deviceIndex < _cfg.numOfDevices && _pendingMaskDevices != 0U; deviceIndex++) {

		if ((_pendingMaskDevices & (1UL << deviceIndex)) == 0U) {
			continue;
		}

		if (BQ_AFE_SetBalanceMask(deviceIndex, _pendingMask[deviceIndex], _cfg.cbTimerCode) != BQ_AFE_STATUS_OK) {
			return;                                   /* TX kuyrugu dolu; sonraki turda devam */
		}

		_appliedMask[deviceIndex] = _pendingMask[deviceIndex];
		_pendingMaskDevices &= ~(1UL << deviceIndex);
	}

	if (_measurementPending) {
		if (BQ_AFE_ReqMeasurementCycle() == BQ_AFE_STATUS_OK) {
			_measurementPending = false;
		}
	}

	if (_clearFaultsPending) {
		if (BQ_AFE_ResetFaults(BQ_AFE_TARGET_STACK) == BQ_AFE_STATUS_OK) {
			_clearFaultsPending = false;
		}
	}
}
