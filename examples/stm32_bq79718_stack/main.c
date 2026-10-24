/*
 * main.c
 *
 *  32 x BQ79718 daisy-chain, IVT CAN akim sensoru, shunt ADC, harici
 *  kontaktorler. RTOS yok, tek superloop.
 *
 *  Uc connector:
 *    BQ_AFE_Connector    : monitor  (hucre, sicaklik, modul V, durum; balance mask hook'u)
 *    Current_Connector   : current  (IVT = birincil, shunt = ikincil; uygulama karari)
 *    Contactor_Connector : contactor (GPIO komut hook'u, aux feedback)
 *
 *  Tur sirasi:
 *    1. Suruculer kendi FSM'lerini ilerletir; veri guncellenince eventFnMap ->
 *       connector -> BMS_Notify* (O(1), kopya yok).
 *    2. BMS_Run(): model, fault, limit, balancing, kontaktor FSM; setpoint
 *       degisirse connector hook'larini cagirir.
 *    3. Connector Update'leri biriken istekleri surucuye aktarir.
 *    4. Uygulama: telemetri, watchdog.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "BMS.h"
#include "BQ_AFE.h"
#include "App_Config.h"
#include "Board.h"
#include "BQ_AFE_Connector.h"
#include "IVT_CAN.h"
#include "SHUNT_ADC.h"
#include "Current_Connector.h"
#include "Contactor_Connector.h"

#define MAIN_CAN_FRAMES_PER_LOOP    16U
#define MAIN_TELEMETRY_PERIOD_MS    100U


static void _drainCanRx(uint32_t tickMs);
static void _telemetryUpdate(uint32_t tickMs);
static void _restoreNvm(void);

static void _onFaultRaised(uint8_t packIndex, uint8_t ruleId, BMS_Severity_t severity);
static void _onFaultCleared(uint8_t packIndex, uint8_t ruleId);
static void _onContactorState(uint8_t packIndex, BMS_ContactorState_t state);
static void _onNvmSaveRequest(uint8_t packIndex);


/* ====================================================================== */
/*  main                                                                  */
/* ====================================================================== */

int main(void) {

	BOARD_Init();

	/* ---- 1. BMS ---- */
	const BMS_PortFnMap_t bmsPortFnMap = {
		.getTickMs	= BOARD_GetTickMs,
		.critEnter	= BOARD_CritEnter,
		.critExit	= BOARD_CritExit,
		.nvmRead	= BOARD_NvmRead,
		.nvmWrite	= BOARD_NvmWrite,
		.log		= BOARD_Log,
	};

	const BMS_EventFnMap_t bmsEventFnMap = {
		.onSetpointChanged	= NULL,
		.onFaultRaised		= _onFaultRaised,
		.onFaultCleared		= _onFaultCleared,
		.onContactorState	= _onContactorState,
		.onNvmSaveRequest	= _onNvmSaveRequest,
	};

	(void)BMS_Init(&appSystemCfgG, &bmsPortFnMap, &bmsEventFnMap);
	_restoreNvm();

	/* ---- 2. Suruculer: donanimi Board'dan, olaylari connector'lardan alirlar ---- */
	const BQ79718_IOFnMap_t bqIoFnMap = {
		.uartTransmit		= BOARD_UartTransmitDma,
		.uartReceiveToIdle	= BOARD_UartReceiveToIdleDma,
		.setUartBaudrate	= BOARD_UartSetBaudrate,
		.getTickMs			= BOARD_GetTickMs,
	};
	const BQ_AFE_EventFnMap_t bqEventFnMap = {
		.onDataUpdated	= BQ_AFE_CONN_OnDataUpdated,
		.onCommError	= BQ_AFE_CONN_OnCommError,
		.onReady		= NULL,
	};
	(void)BQ_AFE_Init(APP_STACK_DEVICE_COUNT, true, BQ_AFE_DEV_78718, &bqIoFnMap, &bqEventFnMap);

	const IVT_CAN_Cfg_t ivtCfg = {
		.baseId			= 0x521,
		.invertCurrent	= false,
	};
	const IVT_CAN_EventFnMap_t ivtEventFnMap = {
		.onDataUpdated	= CURRENT_CONN_OnIvtDataUpdated,
	};
	IVT_CAN_Init(&ivtCfg, &ivtEventFnMap);

	const SHUNT_ADC_Cfg_t shuntCfg = {
		.offsetCounts	= 2048,
		.uaPerCount		= 403000,    /* 100 uOhm shunt, INA240 x20, 12 bit @ 3.3 V */
		.invert			= false,
	};
	const SHUNT_ADC_EventFnMap_t shuntEventFnMap = {
		.onDataUpdated	= CURRENT_CONN_OnShuntDataUpdated,
	};
	SHUNT_ADC_Init(&shuntCfg, &shuntEventFnMap);

	/* ---- 3. Connector'lar: surucu tamponlari BMS'e baglanir, hook'lar kurulur ---- */
	const BQ_AFE_CONN_Cfg_t bqConnCfg = {
		.packIndex				= APP_PACK_INDEX,
		.numOfDevices			= APP_STACK_DEVICE_COUNT,
		.numOfCellsPerDevice	= APP_CELLS_PER_DEVICE,
		.ntcGpioFirst			= APP_NTC_GPIO_FIRST,
		.numOfNtcPerDevice		= APP_NTC_PER_DEVICE,
		.ntcSlotBase			= APP_SLOT_NTC_BASE,
		.dieSlotBase			= APP_SLOT_DIE_BASE,
		.ntc					= &appNtc10kB3435G,
		.tsRefMv				= APP_TSREF_MV,
		.cbTimerCode			= 0x05,    /* CB_CELLx_CTRL kodu, datasheet'e gore */
	};
	(void)BQ_AFE_CONN_Init(&bqConnCfg);

	const CURRENT_CONN_Cfg_t currentConnCfg = {
		.packIndex				= APP_PACK_INDEX,
		.shuntTempSlot			= APP_SLOT_SHUNT,
		.ivtU1IsPack			= true,
		.ivtU2IsBus				= true,
		.ivtUseChargeCounter	= true,
	};
	(void)CURRENT_CONN_Init(&currentConnCfg);

	const CONTACTOR_CONN_Cfg_t contactorConnCfg = {
		.packIndex		= APP_PACK_INDEX,
		.mainNegIndex	= APP_CONTACTOR_MAIN_NEG,
		.prechargeIndex	= APP_CONTACTOR_PRECHARGE,
		.mainPosIndex	= APP_CONTACTOR_MAIN_POS,
	};
	(void)CONTACTOR_CONN_Init(&contactorConnCfg);

	(void)BMS_SetMode(BMS_MODE_ACTIVE);


	/* ==================== SUPERLOOP ==================== */

	for (;;) {

		uint32_t tickMs = BOARD_GetTickMs();

		/* --- 1. suruculer (veri -> eventFnMap -> connector -> BMS_Notify*) --- */
		(void)BQ_AFE_Update();                /* init + olcum FSM, parse                    */
		_drainCanRx(tickMs);                  /* FDCAN ring -> IVT_CAN_OnFrame              */
		SHUNT_ADC_Update(tickMs);             /* ADC DMA -> ortalama                        */
		CONTACTOR_CONN_Update(tickMs);        /* aux feedback oku                           */

		/* --- 2. BMS (setpoint degisirse connector hook'larini cagirir) --- */
		BMS_Run();

		/* --- 3. connector -> surucu (biriken istekler) --- */
		BQ_AFE_CONN_Update();                 /* balance mask -> CB timer'lari              */

		/* --- 4. uygulama --- */
		_telemetryUpdate(tickMs);
		BOARD_WdtKick();
	}
}


/* ====================================================================== */
/*  Yardimcilar                                                           */
/* ====================================================================== */

static void _drainCanRx(uint32_t tickMs) {

	BOARD_CanFrame_t frame;
	uint8_t budget = MAIN_CAN_FRAMES_PER_LOOP;   /* tur suresi sinirli kalsin */

	while (budget-- && BOARD_CanRxPop(&frame)) {
		IVT_CAN_OnFrame(frame.id, frame.data, frame.len, tickMs);
		/* baska CAN kaynaklari (slave pack, sarj cihazi) buraya eklenir */
	}
}


static void _putInt32Be(uint8_t *data, int32_t value) {

	data[0] = (uint8_t)((uint32_t)value >> 24);
	data[1] = (uint8_t)((uint32_t)value >> 16);
	data[2] = (uint8_t)((uint32_t)value >>  8);
	data[3] = (uint8_t)((uint32_t)value);
}

static void _putInt16Be(uint8_t *data, int16_t value) {

	data[0] = (uint8_t)((uint16_t)value >> 8);
	data[1] = (uint8_t)((uint16_t)value);
}


/* Arayuz SDK kapsami disi: burada sadece snapshot'in nasil okundugu gorulsun. */
static void _telemetryUpdate(uint32_t tickMs) {

	static uint32_t lastTickMs = 0;

	if ((uint32_t)(tickMs - lastTickMs) < MAIN_TELEMETRY_PERIOD_MS) {
		return;
	}
	lastTickMs = tickMs;

	const BMS_PackSnapshot_t *snapshot = BMS_GetPackSnapshot(APP_PACK_INDEX);
	const BMS_Setpoint_t     *setpoint = BMS_GetSetpoint(APP_PACK_INDEX);
	uint8_t data[8];

	_putInt32Be(&data[0], snapshot->packVoltMv);
	_putInt32Be(&data[4], snapshot->currentMa);
	(void)BOARD_CanTx(0x600, data, 8);

	_putInt16Be(&data[0], (int16_t)snapshot->socPct);
	_putInt16Be(&data[2], (int16_t)snapshot->sohPct);
	_putInt16Be(&data[4], snapshot->cellMinMv);
	_putInt16Be(&data[6], snapshot->cellMaxMv);
	(void)BOARD_CanTx(0x601, data, 8);

	_putInt32Be(&data[0], setpoint->cclMa);
	_putInt32Be(&data[4], setpoint->dclMa);
	(void)BOARD_CanTx(0x602, data, 8);

	data[0] = (uint8_t)snapshot->severity;
	data[1] = (uint8_t)snapshot->contactorState;
	_putInt32Be(&data[2], (int32_t)snapshot->faultActive);
	_putInt16Be(&data[6], snapshot->tempMaxDeciDeg);
	(void)BOARD_CanTx(0x603, data, 8);

	/* Hucre bazli veri istenirse: BMS_GetCellVoltageMv(APP_PACK_INDEX, i)
	 * connector tamponundan yerinde okur. */
}


/* ====================================================================== */
/*  BMS olaylari                                                          */
/* ====================================================================== */

static void _onFaultRaised(uint8_t packIndex, uint8_t ruleId, BMS_Severity_t severity) {

	BOARD_Logf("pack %u fault %u RAISED sev %u", (unsigned)packIndex, (unsigned)ruleId, (unsigned)severity);
}

static void _onFaultCleared(uint8_t packIndex, uint8_t ruleId) {

	BOARD_Logf("pack %u fault %u cleared", (unsigned)packIndex, (unsigned)ruleId);
}

static void _onContactorState(uint8_t packIndex, BMS_ContactorState_t state) {

	BOARD_Logf("pack %u contactor state %u", (unsigned)packIndex, (unsigned)state);
}

static void _onNvmSaveRequest(uint8_t packIndex) {

	BMS_NvmBlob_t blob;

	if (BMS_ExportNvm(packIndex, &blob) == BMS_STATUS_OK) {
		(void)BOARD_NvmWrite(&blob, sizeof(blob));
	}
}

static void _restoreNvm(void) {

	BMS_NvmBlob_t blob;

	if (BOARD_NvmRead(&blob, sizeof(blob)) &&
	    blob.magic == BMS_NVM_MAGIC && blob.packIndex == APP_PACK_INDEX) {
		(void)BMS_ImportNvm(APP_PACK_INDEX, &blob);
	}
}
