/*
 * App_Config.c
 *
 *  SDK konfigurasyonu: pack topolojisi, fault tablosu, limitler, balancing,
 *  kimya yer tutucusu. Hepsi const (flash).
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "App_Config.h"

#define _R4(x)   x, x, x, x
#define _R8(x)   _R4(x), _R4(x)
#define _R32(x)  _R8(x), _R8(x), _R8(x), _R8(x)


const BMS_Ntc_t appNtc10kB3435G = {
	.r25Ohm		= 10000,
	.betaK		= 3435,
	.rSeriesOhm	= 10000,
	.ntcToGnd	= true,
};

/* Kimya: simdilik yer tutucu. OCV tablosu sonra gelir. */
static const BMS_Chemistry_t _chemPlaceholder = {
	.name			= "placeholder",
	.nominalMah		= 100000,
	.voltMinMv		= 2800,
	.voltMaxMv		= 4200,
	.voltNomMv		= 3650,
	.ocvMv			= NULL,
	.ocvSocPct		= NULL,
	.numOfOcvPoints	= 0,
};

static const BMS_LimitsCfg_t _limits = {
	.cclMaxMa				= 100000,
	.dclMaxMa				= 200000,
	.chgTargetCellMv		= 4150,
	.tempChgMinDeciDeg		= 0,
	.tempChgMaxDeciDeg		= 450,
	.tempDchgMinDeciDeg		= -200,
	.tempDchgMaxDeciDeg		= 550,
	.tempDerateBandDeciDeg	= 50,
	.voltChgTaperStartMv	= 4100,
	.deratePct				= 50,
};

static const BMS_BalanceCfg_t _balance = {
	.deltaStartMv			= 20,
	.deltaStopMv			= 8,
	.minCellMv				= 3400,
	.maxAfeDieTempDeciDeg	= 1000,
	.maxCellTempDeciDeg		= 500,
	.dutyOnPct				= 70,
	.periodMs				= 10000,
	.onlyWhenChargingOrRest	= true,
	.restCurrentMa			= 2000,
	.noAdjacentCells		= true,     /* BQ79718 NO_ADJ_CB ile uyumlu */
	.maxCellsPerModule		= 6,
};

/* Fault tablosu. id = snapshot fault bit'i. Birimler: mV, mA, deciDeg, ms. */
static const BMS_FaultRule_t _faultRules[] = {
	/* id  signal                            compare               threshold  hyst   debounce  severity              actions */
	{  0, BMS_SIGNAL_CELL_VOLT_MAX,         BMS_COMPARE_GT,          4200,     50,    500, BMS_SEVERITY_TRIP,   BMS_ACTION_LIMIT_CHG | BMS_ACTION_OPEN_CONTACTOR | BMS_ACTION_LATCH },
	{  1, BMS_SIGNAL_CELL_VOLT_MAX,         BMS_COMPARE_GT,          4160,     30,   1000, BMS_SEVERITY_WARN,   BMS_ACTION_LIMIT_CHG },
	{  2, BMS_SIGNAL_CELL_VOLT_MIN,         BMS_COMPARE_LT,          2800,     50,    500, BMS_SEVERITY_TRIP,   BMS_ACTION_LIMIT_DCHG | BMS_ACTION_OPEN_CONTACTOR | BMS_ACTION_LATCH },
	{  3, BMS_SIGNAL_CELL_VOLT_MIN,         BMS_COMPARE_LT,          3000,     50,   1000, BMS_SEVERITY_DERATE, BMS_ACTION_LIMIT_DCHG },
	{  4, BMS_SIGNAL_CELL_VOLT_DELTA,       BMS_COMPARE_GT,           300,     50,   5000, BMS_SEVERITY_WARN,   0 },
	{  5, BMS_SIGNAL_TEMP_MAX,              BMS_COMPARE_GT,           600,     30,   1000, BMS_SEVERITY_TRIP,   BMS_ACTION_LIMIT_CHG | BMS_ACTION_LIMIT_DCHG | BMS_ACTION_OPEN_CONTACTOR | BMS_ACTION_LATCH },
	{  6, BMS_SIGNAL_TEMP_MAX,              BMS_COMPARE_GT,           500,     30,   2000, BMS_SEVERITY_DERATE, BMS_ACTION_STOP_BALANCE },
	{  7, BMS_SIGNAL_TEMP_MIN,              BMS_COMPARE_LT,          -200,     30,   2000, BMS_SEVERITY_DERATE, BMS_ACTION_LIMIT_CHG },
	{  8, BMS_SIGNAL_AFE_DIE_TEMP_MAX,      BMS_COMPARE_GT,          1050,     50,   1000, BMS_SEVERITY_WARN,   BMS_ACTION_STOP_BALANCE },
	{  9, BMS_SIGNAL_CURRENT_CHG,           BMS_COMPARE_GT,        120000,  10000,    200, BMS_SEVERITY_TRIP,   BMS_ACTION_OPEN_CONTACTOR | BMS_ACTION_LATCH },
	{ 10, BMS_SIGNAL_CURRENT_DCHG,          BMS_COMPARE_GT,        250000,  10000,    200, BMS_SEVERITY_TRIP,   BMS_ACTION_OPEN_CONTACTOR | BMS_ACTION_LATCH },
	{ 11, BMS_SIGNAL_STALE_CELLS,           BMS_COMPARE_GT,           500,      0,      0, BMS_SEVERITY_TRIP,   BMS_ACTION_OPEN_CONTACTOR },
	{ 12, BMS_SIGNAL_STALE_CURRENT,         BMS_COMPARE_GT,           100,      0,      0, BMS_SEVERITY_TRIP,   BMS_ACTION_OPEN_CONTACTOR },
	{ 13, BMS_SIGNAL_STALE_TEMPS,           BMS_COMPARE_GT,          2000,      0,      0, BMS_SEVERITY_DERATE, BMS_ACTION_STOP_BALANCE },
	{ 14, BMS_SIGNAL_CURRENT_MISMATCH,      BMS_COMPARE_GT,          5000,   1000,   1000, BMS_SEVERITY_WARN,   0 },
	{ 15, BMS_SIGNAL_SUM_VS_MODULE_VOLT,    BMS_COMPARE_GT,           500,    100,   2000, BMS_SEVERITY_WARN,   0 },
	{ 16, BMS_SIGNAL_DEVICE_FAULT,          BMS_COMPARE_ANY_BIT,   0x001F,      0,    200, BMS_SEVERITY_TRIP,   BMS_ACTION_OPEN_CONTACTOR | BMS_ACTION_LATCH },   /* OV|UV|OT|UT|OC */
	{ 17, BMS_SIGNAL_DEVICE_FAULT,          BMS_COMPARE_ANY_BIT,   0x02E0,      0,   1000, BMS_SEVERITY_WARN,   0 },                                                 /* COMM|SYS|PWR|ADC */
};

static const BMS_PackCfg_t _pack0 = {
	.numOfModules			= APP_STACK_DEVICE_COUNT,
	.cellsPerModule			= { _R32(APP_CELLS_PER_DEVICE) },
	.numOfTempSlots			= APP_TEMP_SLOT_COUNT,
	.numOfCurrentSources	= 2,                         /* IVT (primary) + shunt ADC (secondary) */
	.chemistry				= &_chemPlaceholder,
	.limits					= &_limits,
	.balance				= &_balance,
	.faultRules				= _faultRules,
	.numOfFaultRules		= (uint8_t)(sizeof(_faultRules) / sizeof(_faultRules[0])),
	.stale = {
		.cellsMs			= 500,
		.tempsMs			= 2000,
		.currentMs			= 100,
		.packVoltMs			= 200,
		.moduleVoltMs		= 1000,
	},
	.contactor = {
		.enabled			= true,
		.prechargeTimeoutMs	= 2000,
		.prechargeOkPct		= 90,
		.hasFeedback		= true,
		.feedbackTimeoutMs	= 200,
	},
};

const BMS_SystemCfg_t appSystemCfgG = {
	.numOfPacks				= 1,
	.packs					= { &_pack0 },
	.parallelCloseDeltaMv	= 2000,
};
