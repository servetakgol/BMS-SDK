/*
 * bq79718.c
 *
 * BQ79718-specific frame builders and response parser. Called directly by the
 * BQ_AFE core.
 *
 *  Created on: Jun 1, 2026
 *      Author: servet
 */

#include "BQ79718.h"

#include <stddef.h>

#include "BQ79718_RegAddr.h"
#include "BQ79718_RegTypes.h"
#include "../BQ_AFE_Connector.h"

/* ---- Read-request builder ---------------------------------------------- */

bool BQ79718_CreateMsgRequest(BQ_AFE_Request_t request, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	uint16_t regAddr;
	uint16_t size;

	switch (request) {
	case BQ_AFE_REQUEST_CELL_VOLTAGES:	regAddr = BQ79718_ADDR_VCELL18_HI;			size = sizeof(BQ79718_CellVoltagesGroup_t);	break;
	case BQ_AFE_REQUEST_GPIO_VOLTAGES:	regAddr = BQ79718_ADDR_GPIO1_HI;			size = sizeof(BQ79718_GpioVoltagesGroup_t);	break;
	case BQ_AFE_REQUEST_PACK_VOLTAGES:	regAddr = BQ79718_ADDR_VCELL_ACT_SUM_HI;	size = sizeof(BQ79718_PackVoltagesGroup_t);	break;
	case BQ_AFE_REQUEST_DIE_TEMP:		regAddr = BQ79718_ADDR_DIETEMP1_HI;			size = sizeof(BQ79718_DieTempGroup_t);		break;
	case BQ_AFE_REQUEST_DEV_STATUS:		regAddr = BQ79718_ADDR_DEV_STAT1;			size = sizeof(BQ79718_DevStatusGroup_t);	break;
	case BQ_AFE_REQUEST_UNIQ_ID:		regAddr = BQ79718_ADDR_PARTID;				size = sizeof(BQ79718_RegIdGroup_t);		break;
	case BQ_AFE_REQUEST_FAULT_SUMMARY:	regAddr = BQ79718_ADDR_FAULT_SUMMARY;		size = sizeof(BQ79718_RegFaultSummary_t);	break;
	case BQ_AFE_REQUEST_DIR0_ADDR:		regAddr = BQ79718_ADDR_DIR0_ADDR;			size = sizeof(BQ79718_RegDirAddr_t);		break;

	/* No current register is defined in the BQ79718 map (no CURRENT/CC/shunt
	 * address) — current is not sourced from this device. */
	case BQ_AFE_REQUEST_CURRENT:
	default:
		return false;
	}

	return BQ_AFE_FRAME_BuildRequest(target, regAddr, (uint8_t)size, txFrameOut);
}


/* ---- Response parsers -------------------------------------------------- */

static void _parseCellVoltages(const uint8_t *payload, uint16_t size, BQ_AFE_Data_t *out) {

	uint16_t count    = size / 2U;
	uint16_t maxCount = (uint16_t)(sizeof(out->cellVoltagesInMv) / sizeof(out->cellVoltagesInMv[0]));

	if (count > maxCount) {
		count = maxCount;
	}

	for (uint16_t index = 0; index < count; index++) {
		uint16_t raw = (uint16_t)(((uint16_t)payload[2U * index] << 8) | payload[2U * index + 1U]);
		out->cellVoltagesInMv[index] = (int16_t)BQ79718_RAW_TO_MV_CELL(raw);
	}
}

static void _parseGpioVoltages(const uint8_t *payload, uint16_t size, BQ_AFE_Data_t *out) {

	uint16_t count    = size / 2U;
	uint16_t maxCount = (uint16_t)(sizeof(out->gpioVoltagesInMv) / sizeof(out->gpioVoltagesInMv[0]));

	if (count > maxCount) {
		count = maxCount;
	}

	/* todo: GPIO ADC LSB'sini datasheet'ten teyit et (cell ile ayni varsayildi) */
	for (uint16_t index = 0; index < count; index++) {
		uint16_t raw = (uint16_t)(((uint16_t)payload[2U * index] << 8) | payload[2U * index + 1U]);
		out->gpioVoltagesInMv[index] = (int16_t)BQ79718_RAW_TO_MV_CELL(raw);
	}
}

static void _parseDieTemp(const uint8_t *payload, uint16_t size, BQ_AFE_Data_t *out) {

	if (size >= 2U) {
		int16_t raw = (int16_t)(((uint16_t)payload[0] << 8) | payload[1]);
		out->dieTempInC.afeDieTemp1 = (int16_t)BQ79718_RAW_TO_DEGC(raw);
	}

	if (size >= 4U) {
		int16_t raw = (int16_t)(((uint16_t)payload[2] << 8) | payload[3]);
		out->dieTempInC.afeDieTemp2 = (int16_t)BQ79718_RAW_TO_DEGC(raw);
	}
}

static void _parseFaultSummary(const uint8_t *payload, uint16_t size, BQ_AFE_Data_t *out) {

	(void)size;

	BQ79718_RegFaultSummary_t reg = { .raw = payload[0] };

	out->faultSummary.faultPwr     = reg.mapped.faultPwr;
	out->faultSummary.faultSys     = reg.mapped.faultSys;
	out->faultSummary.faultOvuv    = reg.mapped.faultOvuv;
	out->faultSummary.faultOtut    = reg.mapped.faultOtut;
	out->faultSummary.faultComm    = reg.mapped.faultComm;
	out->faultSummary.faultOtp     = reg.mapped.faultOtp;
	out->faultSummary.faultCompAdc = reg.mapped.faultAdcCb;
	out->faultSummary.faultProt    = reg.mapped.faultOc;
}

static void _parsePackVoltages(const uint8_t *payload, uint16_t size, BQ_AFE_Data_t *out) {

	/* todo: VCELL_ACT_SUM LSB'sini datasheet'ten teyit et (VBAT ile ayni varsayildi) */
	if (size >= 2U) {
		uint16_t raw = (uint16_t)(((uint16_t)payload[0] << 8) | payload[1]);
		out->packVoltagesInMv.vcellActSumInMv = BQ79718_RAW_TO_MV_VBAT(raw);
	}

	if (size >= 4U) {
		uint16_t raw = (uint16_t)(((uint16_t)payload[2] << 8) | payload[3]);
		out->packVoltagesInMv.batInMv = BQ79718_RAW_TO_MV_VBAT(raw);
	}
}

static void _parseDevStatus(const uint8_t *payload, uint16_t size, BQ_AFE_Data_t *out) {

	if (size >= 1U) {
		BQ79718_RegDevStat1_t s1 = { .raw = payload[0] };
		out->devStatus.vcellAdcRun = s1.mapped.vcellAdcRun;
		out->devStatus.gpAdcRun    = s1.mapped.gpAdcRun;
		out->devStatus.diagAnaRun  = s1.mapped.diagAnaRun;
		out->devStatus.ovuvRun     = s1.mapped.ovuvRun;
		out->devStatus.otutRun     = s1.mapped.otutRun;
		out->devStatus.custCrcDone = s1.mapped.custCrcDone;
		out->devStatus.factCrcDone = s1.mapped.factCrcDone;
		out->devStatus.diagMeasRun = s1.mapped.diagMeasRun;
	}

	if (size >= 2U) {
		BQ79718_RegDevStat2_t s2 = { .raw = payload[1] };
		out->devStatus.avddOn     = s2.mapped.avddOn;
		out->devStatus.diagDigRun = s2.mapped.diagDigRun;
	}
}

static void _parseDir0Addr(const uint8_t *payload, uint16_t size, BQ_AFE_Data_t *out) {

	if (size < 1U) {
		return;
	}

	BQ79718_RegDirAddr_t reg = { .raw = payload[0] };

	out->dir0Addr = reg.mapped.address;
}

static void _parseId(const uint8_t *payload, uint16_t size, BQ_AFE_Data_t *out) {

	if (size < 10U) {   /* partId(1) + tapeOutRev(1) + dieId(8) */
		return;
	}

	out->id.partId     = payload[0];
	out->id.tapeOutRev = payload[1];

	uint64_t dieId = 0;

	for (int index = 0; index < 8; index++) {
		dieId = (dieId << 8) | payload[2 + index];
	}

	out->id.dieId = dieId;
}


BQ_AFE_MsgId_t BQ79718_Parse(const uint8_t *payload, uint16_t size, uint16_t regAddr, uint8_t deviceIndex, BQ_AFE_Data_t *dataPtr) {

	(void)deviceIndex;   /* dataPtr already targets this device's slot */

	if (payload == NULL || dataPtr == NULL) {
		return BQ_AFE_MSG_NONE;
	}

	switch (regAddr) {

	case BQ79718_ADDR_VCELL18_HI:       _parseCellVoltages(payload, size, dataPtr); return BQ_AFE_MSG_CELL_VOLTAGES;
	case BQ79718_ADDR_GPIO1_HI:         _parseGpioVoltages(payload, size, dataPtr); return BQ_AFE_MSG_GPIO_VOLTAGES;
	case BQ79718_ADDR_FAULT_SUMMARY:    _parseFaultSummary(payload, size, dataPtr); return BQ_AFE_MSG_FAULT_SUMMARY;
	case BQ79718_ADDR_DIETEMP1_HI:      _parseDieTemp(payload, size, dataPtr);      return BQ_AFE_MSG_DIE_TEMP;
	case BQ79718_ADDR_PARTID:           _parseId(payload, size, dataPtr);           return BQ_AFE_MSG_UNIQ_ID;
	case BQ79718_ADDR_DEV_STAT1:        _parseDevStatus(payload, size, dataPtr);    return BQ_AFE_MSG_DEV_STATUS;
	case BQ79718_ADDR_VCELL_ACT_SUM_HI: _parsePackVoltages(payload, size, dataPtr); return BQ_AFE_MSG_PACK_VOLTAGES;
	case BQ79718_ADDR_DIR0_ADDR:        _parseDir0Addr(payload, size, dataPtr);     return BQ_AFE_MSG_DIR0_ADDR;

	default:
		return BQ_AFE_MSG_NONE;
	}
}


/* ---- Config encoders + builders --------------------------------------- */

static uint8_t _encNumCell(uint8_t cells) {

	if (cells < 9U) {
		cells = 9U;
	}

	if (cells > 18U) {
		cells = 18U;
	}

	return (uint8_t)(cells - 6U);
}

static uint8_t _encOvThresh(uint16_t mv) {

	if (mv <= 3000U) {

		if (mv < 2700U) {
			mv = 2700U;
		}

		return (uint8_t)((mv - 2700U) / 25U);
	}

	if (mv <= 4000U) {

		if (mv < 3600U) {
			mv = 3600U;
		}

		return (uint8_t)(0x40U + (mv - 3600U) / 25U);
	}

	if (mv < 4175U) {
		mv = 4175U;
	}

	if (mv > 5100U) {
		mv = 5100U;
	}

	return (uint8_t)(0x80U + (mv - 4175U) / 25U);
}

static uint8_t _encUvThresh(uint16_t mv) {

	if (mv < 1200U) {
		mv = 1200U;
	}

	if (mv > 3700U) {
		mv = 3700U;
	}

	return (uint8_t)((mv - 1200U) / 50U);
}

/* TODO: OT/UT thresholds depend on the NTC/TSREF divider, not a plain
 * degree->code formula. Until the thermistor curve is wired in, the raw degree
 * is just clamped into the field width as a placeholder. */
static uint8_t _encOtThresh(int16_t degree) {

	if (degree < 0)    degree = 0;
	if (degree > 0x1F) degree = 0x1F;   /* otThr is 5 bits */
	return (uint8_t)degree;
}

static uint8_t _encUtThresh(int16_t degree) {

	if (degree < 0)    degree = 0;
	if (degree > 0x07) degree = 0x07;   /* utThr is 3 bits */
	return (uint8_t)degree;
}


/* DEV_CONF1 (0x002) — heartbeat / fault tone / nFAULT / CB warnings / sleep cycle. */
bool BQ79718_CreateMsgDevConfig1(const BQ_AFE_DevConfig1_t *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	BQ79718_RegDevConf1_t r = { .raw = 0 };
	r.mapped.hbEn         = cfg->heartBeatEnabled       ? 1U : 0U;
	r.mapped.ftoneEn      = cfg->faultToneEnabled       ? 1U : 0U;
	r.mapped.nfaultEn     = cfg->nFaultEnabled          ? 1U : 0U;
	r.mapped.noAdjCb      = cfg->cbAdjacentCellDisabled ? 1U : 0U;
	r.mapped.cbtwarnDis   = cfg->cbTwarnDisabled        ? 1U : 0U;
	r.mapped.twarnThr     = (uint8_t)(cfg->twarnTreshold        & 0x01U);
	r.mapped.protSlpCycle = (uint8_t)(cfg->protectionSleepCycle & 0x03U);

	uint8_t data = r.raw;
	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_DEV_CONF1, &data, 1U, txFrameOut);
}

/* DEV_CONF2 (0x003) — cell count, busbar pin, PTC. */
bool BQ79718_CreateMsgDevConfig2(const BQ_AFE_DevConfig2_t *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	BQ79718_RegDevConf2_t r = { .raw = 0 };
	r.mapped.numCell  = _encNumCell(cfg->numOfCellCount);
	r.mapped.bbPinLoc = (uint8_t)(cfg->bbPinLoc & 0x03U);
	r.mapped.bbPinEn  = cfg->bbPinEnabled ? 1U : 0U;
	r.mapped.ptcEn    = cfg->ptcEnabled   ? 1U : 0U;

	uint8_t data = r.raw;
	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_DEV_CONF2, &data, 1U, txFrameOut);
}

/* ADC_CONF (0x008) — sample delay (register unit is 4 us). */
bool BQ79718_CreateMsgAdcConfig(const BQ_AFE_AdcConfig_t *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	uint8_t data = (uint8_t)(cfg->adcDelayUs / 4U);
	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_ADC_CONF, &data, 1U, txFrameOut);
}

/* OV_THRESH (0x009), UV_THRESH (0x00A), OTUT_THRESH (0x00B) are contiguous, so the
 * whole protection-threshold group goes out in one 3-byte write. */
bool BQ79718_CreateMsgProtConfigGroup(const BQ_AFE_ProtConfigGroup_t *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	BQ79718_ProtThreshGroup_t group = {0};

	group.ovThresh.mapped.ovThr   = _encOvThresh(cfg->ovTresholdMv);
	group.uvThresh.mapped.uvThr   = _encUvThresh(cfg->uvTresholdMv);
	group.otutThresh.mapped.otThr = _encOtThresh(cfg->otTresholdDegree);
	group.otutThresh.mapped.utThr = _encUtThresh(cfg->utTresholdDegree);

	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_OV_THRESH, (uint8_t *)&group, sizeof(group), txFrameOut);
}

/* ADC_CTRL2 (0x311) — ADC mode / VCELL LPF / freeze, asserts ADC_GO. */
bool BQ79718_CreateMsgAdcCtrl(const BQ_AFE_AdcCtrlGroup_t *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	BQ79718_RegAdcCtrl2_t r = { .raw = 0 };
	r.mapped.adcGo      = cfg->startAdc      ? 1U : 0U;
	r.mapped.adcMode    = (uint8_t)cfg->adcMode;
	r.mapped.lpfVcellEn = cfg->lpfEnabled    ? 1U : 0U;
	r.mapped.lpfVcell   = (uint8_t)cfg->lpfFilter;
	r.mapped.freezeEn   = cfg->freezeEnabled ? 1U : 0U;

	uint8_t data = r.raw;
	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_ADC_CTRL2, &data, 1U, txFrameOut);
}

/* OVUV_CTRL1 (0x334) .. OTUT_CTRL2 (0x337) are contiguous, so one 4-byte write sets
 * both protection modes and kicks their GO bits. Lock fields are left unlocked. */
bool BQ79718_CreateMsgProtectionCtrlGroup(const BQ_AFE_ProtCtrlGroup_t *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	BQ79718_ProtectionCtrlGroup_t group = {0};

	group.ovuvCtrl2.mapped.ovuvMode = (uint8_t)(cfg->ovuvMode & 0x03U);
	group.ovuvCtrl2.mapped.ovuvGo   = (cfg->ovuvMode != 0U) ? 1U : 0U;

	group.otutCtrl2.mapped.otutMode = (uint8_t)(cfg->otutMode & 0x03U);
	group.otutCtrl2.mapped.otutGo   = (cfg->otutMode != 0U) ? 1U : 0U;

	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_OVUV_CTRL1, (uint8_t *)&group, sizeof(group), txFrameOut);
}


/* ---- Auto-addressing builders ----------------------------------------- */

bool BQ79718_CreateMsgOtpEccTestWrite(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	uint16_t regAddr = (uint16_t)(BQ79718_ADDR_OTP_ECC_TEST);
	uint8_t  data    = 0x00U;
	return BQ_AFE_FRAME_BuildWrite(target, regAddr, &data, 1U, txFrameOut);
}

bool BQ79718_CreateMsgOtpEccTestRead(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	uint16_t regAddr = (uint16_t)(BQ79718_ADDR_OTP_ECC_TEST);
	return BQ_AFE_FRAME_BuildRequest(target, regAddr, 1U, txFrameOut);
}


bool BQ79718_CreateMsgSetDir0DevAddr(uint8_t addr, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	BQ79718_RegDirAddr_t dir = { .raw = 0 };
	dir.mapped.address = (uint8_t)(addr & 0x3FU);   /* 6-bit ring address */

	uint8_t data = dir.raw;
	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_DIR0_ADDR, &data, 1U, txFrameOut);
}

bool BQ79718_CreateMsgSetDir1DevAddr(uint8_t addr, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	BQ79718_RegDirAddr_t dir = { .raw = 0 };
	dir.mapped.address = (uint8_t)(addr & 0x3FU);   /* 6-bit ring address */

	uint8_t data = dir.raw;
	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_DIR1_ADDR, &data, 1U, txFrameOut);
}

bool BQ79718_CreateMsgSetTopStack(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	BQ79718_RegCommCtrl_t commCtrl = { .raw = 0 };
	commCtrl.mapped.topStack = 1U;

	uint8_t data = commCtrl.raw;
	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_COMM_CTRL, &data, 1U, txFrameOut);
}

bool BQ79718_CreateMsgSetStackDev(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	BQ79718_RegCommCtrl_t commCtrl = { .raw = 0x02 }; // stackDevice = 1 // topstack = 0

	uint8_t data = commCtrl.raw;
	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_COMM_CTRL, &data, 1U, txFrameOut);
}


/* ---- Control command + fault reset ------------------------------------ */

bool BQ79718_CreateMsgCtrlCmd(BQ_AFE_CtrlCmd cmd, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	if (txFrameOut == NULL) {
		return false;
	}

	uint16_t regAddr;
	uint8_t  data;

	switch (cmd) {

		case BQ_AFE_CTRL_CMD_DIR_SEL:         regAddr = BQ79718_ADDR_CONTROL1; data = (BQ79718_RegControl1_t){ .mapped.dirSel       = 1U }.raw; break;
		case BQ_AFE_CTRL_CMD_HW_RESET:        regAddr = BQ79718_ADDR_CONTROL1; data = (BQ79718_RegControl1_t){ .mapped.sendSdHwRst  = 1U }.raw; break;
		case BQ_AFE_CTRL_CMD_WAKE_NEXT_CHAIN: regAddr = BQ79718_ADDR_CONTROL1; data = (BQ79718_RegControl1_t){ .mapped.sendWake     = 1U }.raw; break;
		case BQ_AFE_CTRL_CMD_SLEEP_TO_ACT:    regAddr = BQ79718_ADDR_CONTROL1; data = (BQ79718_RegControl1_t){ .mapped.sendSlptoAct = 1U }.raw; break;
		case BQ_AFE_CTRL_CMD_SHUTDOWN:        regAddr = BQ79718_ADDR_CONTROL1; data = (BQ79718_RegControl1_t){ .mapped.gotoShutdown = 1U }.raw; break;
		case BQ_AFE_CTRL_CMD_SLEEP:           regAddr = BQ79718_ADDR_CONTROL1; data = (BQ79718_RegControl1_t){ .mapped.gotoSleep    = 1U }.raw; break;
		case BQ_AFE_CTRL_CMD_SOFT_RESET:      regAddr = BQ79718_ADDR_CONTROL1; data = (BQ79718_RegControl1_t){ .mapped.softReset    = 1U }.raw; break;
		case BQ_AFE_CTRL_CMD_ADDR_WR:         regAddr = BQ79718_ADDR_CONTROL1; data = (BQ79718_RegControl1_t){ .mapped.addrWr       = 1U }.raw; break;

		case BQ_AFE_CTRL_CMD_TSREF_EN:        regAddr = BQ79718_ADDR_CONTROL2; data = (BQ79718_RegControl2_t){ .mapped.tsRefEn      = 1U }.raw; break;
		case BQ_AFE_CTRL_CMD_I2C_EN:          regAddr = BQ79718_ADDR_CONTROL2; data = (BQ79718_RegControl2_t){ .mapped.i2cEn        = 1U }.raw; break;

		default:
			return false;
	}

	return BQ_AFE_FRAME_BuildWrite(target, regAddr, &data, 1U, txFrameOut);
}


bool BQ79718_CreateMsgResetFaults(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	if (txFrameOut == NULL) {
		return false;
	}

	/* Write-1-to-clear every fault group. FAULT_RST1 (0x340) and FAULT_RST2
	 * (0x341) are contiguous, so both clear in a single 2-byte write. */
	uint8_t data[2] = { 0xFFU,   /* FAULT_RST1: rstPwr..rstOc       */
	                    0xFFU }; /* FAULT_RST2: rstCommUart..rstAdc */
	return BQ_AFE_FRAME_BuildWrite(target, BQ79718_ADDR_FAULT_RST1, data, 2U, txFrameOut);
}


/* ---- Stack wake --------------------------------------------------------- */

bool BQ79718_CreateMsgStackWake1(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	uint8_t data = 0xA5U;
	return BQ_AFE_FRAME_BuildWrite(target, 0x0700U, &data, 1U, txFrameOut);
}

bool BQ79718_CreateMsgStackWake2(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut) {

	uint8_t data = 0x20U;
	return BQ_AFE_FRAME_BuildWrite(target, 0x0701U, &data, 1U, txFrameOut);
}


/* ---- Connector ---------------------------------------------------------- */

static const BQ_AFE_Connector_t _bq79718Connector = {

	.createMsgRequest         = BQ79718_CreateMsgRequest,
	.createMsgCtrlCmd         = BQ79718_CreateMsgCtrlCmd,
	.parse                    = BQ79718_Parse,

	.createMsgStackWake1      = BQ79718_CreateMsgStackWake1,
	.createMsgStackWake2      = BQ79718_CreateMsgStackWake2,

	.createMsgOtpEccTestWrite = BQ79718_CreateMsgOtpEccTestWrite,
	.createMsgOtpEccTestRead  = BQ79718_CreateMsgOtpEccTestRead,
	.createMsgDeviceAddrDir0  = BQ79718_CreateMsgSetDir0DevAddr,
	.createMsgDeviceAddrDir1  = BQ79718_CreateMsgSetDir1DevAddr,
	.createMsgSetTopStack     = BQ79718_CreateMsgSetTopStack,

	.createMsgResetFaults     = BQ79718_CreateMsgResetFaults,
	.createMsgDevConfig1      = BQ79718_CreateMsgDevConfig1,
	.createMsgDevConfig2      = BQ79718_CreateMsgDevConfig2,
	.createMsgProtConfig      = BQ79718_CreateMsgProtConfigGroup,
	.createMsgAdcConfig       = BQ79718_CreateMsgAdcConfig,
	.createMsgAdcCtrl         = BQ79718_CreateMsgAdcCtrl,
	.createMsgProtCtrl        = BQ79718_CreateMsgProtectionCtrlGroup,

	.maxCellCount = 18U,
	.maxGpioCount = 11U,
	.dir0AddrReg  = BQ79718_ADDR_DIR0_ADDR,
};


const BQ_AFE_Connector_t *BQ79718_GetConnector(void) {

	return &_bq79718Connector;
}
