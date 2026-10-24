/*
 * BMS_Out.h
 *
 *  SDK cikisi: setpoint, snapshot, olay fonksiyon haritasi.
 *
 *  Setpoint : BMS'in dis dunyadan istedigi sey. Balance mask, kontaktor
 *             komutu, FET izinleri connector hook'lariyla BMS tarafindan
 *             uygulanir; burada okunmasi bilgi amaclidir (telemetri, sarj
 *             cihazi limitleri).
 *  Snapshot : uygulama / telemetri icin ozet. Hucre bazli degerler erisimci
 *             ile okunur; bu, connector tamponundan yerinde okumadir.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef BMS_OUT_H_
#define BMS_OUT_H_

#include <stdbool.h>
#include <stdint.h>

#include "BMS_Types.h"


/* ---- Setpoint --------------------------------------------------------- */

typedef struct {
	uint32_t				seq;                                  /* her degisimde artar */

	uint32_t				balanceMask[BMS_CFG_MAX_MODULES];     /* bit i = hucre i+1   */

	bool					chgAllowed;
	bool					dchgAllowed;
	int32_t					cclMa;                                /* charge current limit    */
	int32_t					dclMa;                                /* discharge current limit */
	int32_t					chgTargetMv;                          /* pack sarj hedefi        */

	BMS_ContactorCmd_t		contactorCmd;                         /* harici kontaktor        */
	bool					fetChgEnabled;                        /* entegre FET'li AFE'ler  */
	bool					fetDchgEnabled;

	BMS_Severity_t			severity;
}BMS_Setpoint_t;


/* ---- Snapshot --------------------------------------------------------- */

#define BMS_STALE_CELLS         (1U << 0)
#define BMS_STALE_TEMPS         (1U << 1)
#define BMS_STALE_CURRENT       (1U << 2)
#define BMS_STALE_PACK_VOLT     (1U << 3)
#define BMS_STALE_MODULE_VOLT   (1U << 4)

typedef struct {
	uint32_t				tickMs;              /* snapshot'in uretildigi tur             */
	uint16_t				numOfCells;

	int16_t					cellMinMv;
	int16_t					cellMaxMv;
	int16_t					cellAvgMv;
	int16_t					cellDeltaMv;
	uint16_t				cellMinIndex;        /* pack-global hucre index                */
	uint16_t				cellMaxIndex;
	int32_t					cellSumMv;
	int32_t					packVoltMv;          /* olculen pack (varsa)                   */
	int32_t					busVoltMv;

	int32_t					currentMa;           /* birincil kaynak                        */
	int32_t					currentSecondaryMa;

	int16_t					tempMinDeciDeg;      /* rol CELL / MODULE / BUSBAR             */
	int16_t					tempMaxDeciDeg;
	uint16_t				tempMinSlot;
	uint16_t				tempMaxSlot;
	int16_t					afeDieTempMaxDeciDeg;
	int16_t					shuntTempDeciDeg;

	uint16_t				socPct;              /* 0.01 %                                 */
	uint16_t				sohPct;
	uint32_t				cycleCount;
	int64_t					chargeMas;           /* CC akumulatoru                         */

	uint32_t				faultActive;         /* bit = kural id                         */
	uint32_t				faultLatched;
	BMS_Severity_t			severity;
	BMS_ContactorState_t	contactorState;
	uint32_t				staleMask;           /* BMS_STALE_*                            */
	BMS_DeviceFaultFlags_t	deviceFaultOr;       /* tum modullerin OR'u                    */

	bool					balancingActive;
	uint16_t				numOfBalancingCells;
	BMS_Mode_t				mode;
}BMS_PackSnapshot_t;

typedef struct {
	uint32_t				tickMs;
	uint8_t					numOfPacks;
	uint8_t					numOfPacksOnline;
	int32_t					totalCurrentMa;
	int32_t					cclMa;               /* sistem toplami                         */
	int32_t					dclMa;
	uint16_t				socPct;              /* Ah agirlikli                           */
	BMS_Severity_t			severity;
}BMS_SystemSnapshot_t;

const BMS_Setpoint_t		*BMS_GetSetpoint       (uint8_t packIndex);
const BMS_PackSnapshot_t	*BMS_GetPackSnapshot   (uint8_t packIndex);
const BMS_SystemSnapshot_t	*BMS_GetSystemSnapshot (void);


/* ---- Hucre / sicaklik erisimcileri ------------------------------------ */

typedef union {
	uint8_t raw;
	struct {
		uint8_t overVolt   :1;
		uint8_t underVolt  :1;
		uint8_t balancing  :1;
		uint8_t stale      :1;
		uint8_t suspect    :1;
		uint8_t reserved   :3;
	} mapped;
}BMS_CellFlags_t;

int16_t			BMS_GetCellVoltageMv  (uint8_t packIndex, uint16_t cellIndex);    /* pack-global index */
BMS_CellFlags_t	BMS_GetCellFlags      (uint8_t packIndex, uint16_t cellIndex);
int16_t			BMS_GetTempDeciDeg    (uint8_t packIndex, uint16_t slot);
uint16_t		BMS_GetCellIndex      (uint8_t packIndex, uint8_t moduleIndex, uint8_t cellInModule);


/* ---- Olay fonksiyon haritasi (SDK -> uygulama) ------------------------ */

typedef struct {
	void	(*onSetpointChanged)	(uint8_t packIndex, const BMS_Setpoint_t *setpoint);   /* limit/izin degisimi icin (sarj cihazi vb.) */
	void	(*onFaultRaised)		(uint8_t packIndex, uint8_t ruleId, BMS_Severity_t severity);
	void	(*onFaultCleared)		(uint8_t packIndex, uint8_t ruleId);
	void	(*onContactorState)		(uint8_t packIndex, BMS_ContactorState_t state);
	void	(*onNvmSaveRequest)		(uint8_t packIndex);                                     /* uygulama BMS_ExportNvm cagirir */
}BMS_EventFnMap_t;


#endif /* BMS_OUT_H_ */
