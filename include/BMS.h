/*
 * BMS.h
 *
 *  Battery Manager SDK tek giris basligi.
 *
 *  Kullanim:
 *    BMS_Init(&systemCfg, &portFnMap, &eventFnMap);
 *    BMS_Attach*Connector(...)             // kaynak tamponlari + hook'lar
 *    for (;;) { ...; BMS_Run(); ... }      // superloop
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef BMS_H_
#define BMS_H_

#include <stdbool.h>
#include <stdint.h>

#include "BMS_Types.h"
#include "BMS_Cfg.h"
#include "BMS_Connector.h"
#include "BMS_Out.h"


#define BMS_SDK_VERSION_MAJOR   0U
#define BMS_SDK_VERSION_MINOR   1U


BMS_Status_t BMS_Init(const BMS_SystemCfg_t *systemCfg, const BMS_PortFnMap_t *portFnMap, const BMS_EventFnMap_t *eventFnMap);

/* Superloop'tan her tur cagrilir. Bloklamaz. Dirty veri yoksa hizla doner.
 * Setpoint degisirse connector hook'larini buradan cagirir. */
void BMS_Run(void);


/* ---- Komutlar (uygulama -> SDK) --------------------------------------- */

BMS_Status_t BMS_RequestContactor	(uint8_t packIndex, BMS_ContactorReq_t request);
BMS_Status_t BMS_SetMode			(BMS_Mode_t mode);
BMS_Status_t BMS_ClearFaults		(uint8_t packIndex, uint32_t ruleMask);
BMS_Status_t BMS_SetBalanceEnabled	(uint8_t packIndex, bool enabled);
BMS_Status_t BMS_RequestMeasurement	(uint8_t packIndex);                     /* tum monitor connector'lara iletir */


/* ---- Kalici durum (SDK blob verir, yazma karari ve ortam uygulamada) -- */

#define BMS_NVM_MAGIC   UINT32_C(0x424D5331)   /* "BMS1" */

typedef struct {
	uint32_t	magic;
	uint16_t	version;
	uint8_t		packIndex;
	uint8_t		reserved;
	uint16_t	socPct;
	uint16_t	sohPct;
	int64_t		chargeMas;
	uint32_t	cycleCount;
	uint32_t	capacityEstMah;
	uint32_t	crc32;                    /* bu alan haric tum blob */
}BMS_NvmBlob_t;

BMS_Status_t BMS_ExportNvm(uint8_t packIndex, BMS_NvmBlob_t *blobOut);
BMS_Status_t BMS_ImportNvm(uint8_t packIndex, const BMS_NvmBlob_t *blob);   /* Init'ten sonra, Run'dan once */


#endif /* BMS_H_ */
