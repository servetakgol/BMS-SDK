/*
 * BQ_AFE_Connector.h
 *
 *  BQ_AFE surucusu <-> BMS monitor connector'i.
 *
 *  Surucu veriyi kendi tamponunda tutar. Init'te her cihaz icin bir
 *  BMS_MonitorConnector_t kurulur (pointer'lar + hook'lar). Veri geldiginde
 *  surucunun eventFnMap.onDataUpdated callback'i buraya gelir ve sadece
 *  BMS_NotifyMonitor cagrilir. Balance mask ise BMS'in hook'u cagirmasiyla
 *  surucuye gider; surucu mesgulse Update'te yeniden denenir.
 *
 *  NOT: Surucunun hedef API'sini kullanir (BQ_AFE_REVIEW.md D2, D5, D7, D8).
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef BQ_AFE_CONNECTOR_H_
#define BQ_AFE_CONNECTOR_H_

#include <stdbool.h>
#include <stdint.h>

#include "BMS.h"
#include "BQ_AFE.h"

typedef struct {
	uint8_t			packIndex;
	uint8_t			numOfDevices;           /* stack'teki cihaz sayisi               */
	uint8_t			numOfCellsPerDevice;
	uint8_t			ntcGpioFirst;           /* termistorlerin ilk GPIO'su (0 tabanli)*/
	uint8_t			numOfNtcPerDevice;
	uint16_t		ntcSlotBase;            /* BMS slot: base + dev*count + k        */
	uint16_t		dieSlotBase;            /* BMS slot: base + dev*2 + k            */
	const BMS_Ntc_t	*ntc;
	uint16_t		tsRefMv;
	uint8_t			cbTimerCode;            /* CB_CELLx_CTRL sure kodu               */
}BQ_AFE_CONN_Cfg_t;

/* BMS_Init ve BQ_AFE_Init'ten SONRA: connector'lari kurar. */
bool BQ_AFE_CONN_Init(const BQ_AFE_CONN_Cfg_t *cfg);

/* Surucunun BQ_AFE_EventFnMap_t'sine takilir. Surucu parse baglaminda (ana dongu) cagirir. */
void BQ_AFE_CONN_OnDataUpdated	(uint8_t deviceIndex, BQ_AFE_DataGroup_t group, const BQ_AFE_Data_t *data, uint32_t tickMs);
void BQ_AFE_CONN_OnCommError	(uint8_t deviceIndex, uint32_t tickMs);

/* Superloop'tan her tur: BMS hook'larinin biriktirdigi istekleri surucuye aktarir. */
void BQ_AFE_CONN_Update(void);

#endif /* BQ_AFE_CONNECTOR_H_ */
