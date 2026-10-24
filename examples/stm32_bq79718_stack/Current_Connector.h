/*
 * Current_Connector.h
 *
 *  Akim kaynaklari <-> BMS current connector'lari. UYGULAMA karari:
 *  birincil kaynak IVT (CAN), ikincil kaynak shunt (ADC). BMS hangisinin
 *  ne oldugunu bilmez; sadece iki kaynak gorur ve karsilastirir.
 *
 *  Init'te surucu tamponlari BMS'e baglanir. Suruculerin eventFnMap
 *  callback'leri buraya gelir ve BMS_NotifyCurrent cagrilir.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef CURRENT_CONNECTOR_H_
#define CURRENT_CONNECTOR_H_

#include <stdbool.h>
#include <stdint.h>

#include "BMS.h"

typedef struct {
	uint8_t		packIndex;
	uint16_t	shuntTempSlot;        /* IVT T kanali icin BMS slot'u            */
	bool		ivtU1IsPack;          /* IVT U1 = pack (batarya) tarafi          */
	bool		ivtU2IsBus;           /* IVT U2 = bus (yuk) tarafi               */
	bool		ivtUseChargeCounter;  /* IVT As sayacini CC kaynagi yap          */
}CURRENT_CONN_Cfg_t;

/* BMS_Init, IVT_CAN_Init, SHUNT_ADC_Init'ten SONRA. */
bool CURRENT_CONN_Init(const CURRENT_CONN_Cfg_t *cfg);

/* IVT_CAN_EventFnMap_t.onDataUpdated ve SHUNT_ADC_EventFnMap_t.onDataUpdated hedefleri */
void CURRENT_CONN_OnIvtDataUpdated  (uint32_t ivtDataMask, uint32_t tickMs);
void CURRENT_CONN_OnShuntDataUpdated(uint32_t tickMs);

#endif /* CURRENT_CONNECTOR_H_ */
