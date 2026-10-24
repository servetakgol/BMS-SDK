/*
 * BMS_Cfg.h
 *
 *  Pack / sistem konfigurasyon descriptor'lari. Hepsi const, derleme zamani.
 *  SDK bunlara sadece pointer tutar.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef BMS_CFG_H_
#define BMS_CFG_H_

#include <stdbool.h>
#include <stdint.h>

#include "BMS_Types.h"


/* ---- Fault kurallari -------------------------------------------------- */

typedef enum {
	BMS_SIGNAL_CELL_VOLT_MAX = 0,     /* mV                                         */
	BMS_SIGNAL_CELL_VOLT_MIN,         /* mV                                         */
	BMS_SIGNAL_CELL_VOLT_DELTA,       /* mV                                         */
	BMS_SIGNAL_PACK_VOLT,             /* mV                                         */
	BMS_SIGNAL_CURRENT_CHG,           /* mA, pozitif                                */
	BMS_SIGNAL_CURRENT_DCHG,          /* mA, pozitif                                */
	BMS_SIGNAL_TEMP_MAX,              /* deciDeg, rol CELL/MODULE/BUSBAR            */
	BMS_SIGNAL_TEMP_MIN,              /* deciDeg                                    */
	BMS_SIGNAL_AFE_DIE_TEMP_MAX,      /* deciDeg                                    */
	BMS_SIGNAL_SHUNT_TEMP_MAX,        /* deciDeg                                    */
	BMS_SIGNAL_STALE_CELLS,           /* ms: en eski modulun yasi                   */
	BMS_SIGNAL_STALE_TEMPS,           /* ms                                         */
	BMS_SIGNAL_STALE_CURRENT,         /* ms                                         */
	BMS_SIGNAL_STALE_PACK_VOLT,       /* ms                                         */
	BMS_SIGNAL_CURRENT_MISMATCH,      /* mA: |primary - secondary|                  */
	BMS_SIGNAL_SUM_VS_MODULE_VOLT,    /* mV: |sum(cells) - moduleVolt|, en kotu modul */
	BMS_SIGNAL_SUM_VS_PACK_VOLT,      /* mV                                         */
	BMS_SIGNAL_DEVICE_FAULT,          /* bitmask: BMS_DeviceFaultFlags_t.raw        */
	BMS_SIGNAL_COUNT
}BMS_Signal_t;

typedef enum {
	BMS_COMPARE_GT = 0,
	BMS_COMPARE_LT,
	BMS_COMPARE_ANY_BIT,
}BMS_Compare_t;

#define BMS_ACTION_LIMIT_CHG        (1U << 0)   /* chgAllowed = false          */
#define BMS_ACTION_LIMIT_DCHG       (1U << 1)   /* dchgAllowed = false         */
#define BMS_ACTION_STOP_BALANCE     (1U << 2)
#define BMS_ACTION_OPEN_CONTACTOR   (1U << 3)
#define BMS_ACTION_LATCH            (1U << 4)   /* BMS_ClearFaults gerekir     */

typedef struct {
	uint8_t			id;             /* 0..31; snapshot faultActive bit'i          */
	BMS_Signal_t	signal;
	BMS_Compare_t	compare;
	int32_t			threshold;      /* sinyalin birimi                            */
	int32_t			hysteresis;     /* geri donus bandi, ayni birim               */
	uint16_t		debounceMs;
	BMS_Severity_t	severity;
	uint16_t		actions;        /* BMS_ACTION_*                               */
}BMS_FaultRule_t;


/* ---- Balancing -------------------------------------------------------- */

typedef struct {
	int16_t		deltaStartMv;            /* vCell - vMin bunu asarsa aday            */
	int16_t		deltaStopMv;             /* bunun altina inince birak                */
	int16_t		minCellMv;               /* bu voltajin altindaki hucre dengelenmez  */
	int16_t		maxAfeDieTempDeciDeg;    /* AFE_DIE bunu asarsa o modulde durdur     */
	int16_t		maxCellTempDeciDeg;
	uint8_t		dutyOnPct;               /* periodMs icinde acik yuzde (0..100)      */
	uint32_t	periodMs;
	bool		onlyWhenChargingOrRest;
	int32_t		restCurrentMa;           /* |I| bunun altindaysa dinlenme            */
	bool		noAdjacentCells;         /* komsu hucreler ayni anda dengelenmez     */
	uint8_t		maxCellsPerModule;       /* AFE termal butcesi; 0 = sinirsiz         */
}BMS_BalanceCfg_t;


/* ---- Limitler --------------------------------------------------------- */

typedef struct {
	int32_t		cclMaxMa;
	int32_t		dclMaxMa;
	int16_t		chgTargetCellMv;         /* chgTargetMv = bu x seri hucre sayisi     */
	int16_t		tempChgMinDeciDeg;
	int16_t		tempChgMaxDeciDeg;
	int16_t		tempDchgMinDeciDeg;
	int16_t		tempDchgMaxDeciDeg;
	int16_t		tempDerateBandDeciDeg;   /* sinira bu kadar yaklasinca lineer derate */
	int16_t		voltChgTaperStartMv;     /* max hucre bunu gecince CCL lineer duser  */
	uint8_t		deratePct;               /* BMS_SEVERITY_DERATE'te limit carpani, %  */
}BMS_LimitsCfg_t;


/* ---- Staleness -------------------------------------------------------- */

typedef struct {
	uint32_t	cellsMs;
	uint32_t	tempsMs;
	uint32_t	currentMs;
	uint32_t	packVoltMs;
	uint32_t	moduleVoltMs;
}BMS_StaleCfg_t;


/* ---- Kontaktor (opsiyonel modul) -------------------------------------- */

typedef struct {
	bool		enabled;
	uint32_t	prechargeTimeoutMs;
	uint8_t		prechargeOkPct;          /* busVolt >= packVolt x pct/100 ise ana kapat */
	bool		hasFeedback;
	uint32_t	feedbackTimeoutMs;
}BMS_ContactorCfg_t;


/* ---- Kimya (yer tutucu; OCV tablolari sonra) -------------------------- */

typedef struct {
	const char		*name;
	uint32_t		nominalMah;
	int16_t			voltMinMv;
	int16_t			voltMaxMv;
	int16_t			voltNomMv;
	const int16_t	*ocvMv;              /* artan sirada, numOfOcvPoints adet; NULL = yok */
	const uint16_t	*ocvSocPct;          /* 0.01 %                                        */
	uint8_t			numOfOcvPoints;
}BMS_Chemistry_t;


/* ---- Pack ve sistem --------------------------------------------------- */

typedef struct {
	uint8_t					numOfModules;
	uint8_t					cellsPerModule[BMS_CFG_MAX_MODULES];
	uint16_t				numOfTempSlots;
	uint8_t					numOfCurrentSources;     /* 1 veya 2                          */
	const BMS_Chemistry_t	*chemistry;
	const BMS_LimitsCfg_t	*limits;
	const BMS_BalanceCfg_t	*balance;
	const BMS_FaultRule_t	*faultRules;
	uint8_t					numOfFaultRules;
	BMS_StaleCfg_t			stale;
	BMS_ContactorCfg_t		contactor;
}BMS_PackCfg_t;

typedef struct {
	uint8_t					numOfPacks;
	const BMS_PackCfg_t		*packs[BMS_CFG_MAX_PACKS];
	int32_t					parallelCloseDeltaMv;    /* paralel pack baglama esigi        */
}BMS_SystemCfg_t;


#endif /* BMS_CFG_H_ */
