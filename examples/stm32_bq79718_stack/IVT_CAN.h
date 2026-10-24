/*
 * IVT_CAN.h
 *
 *  Isabellenhuette IVT-S / IVT-MOD CAN akim sensoru surucusu.
 *
 *  BQ_AFE ile ayni desen: veri kendi tamponunda (IVT_CAN_Data_t), guncellenince
 *  eventFnMap.onDataUpdated. BMS'i bilmez; Current_Connector baglar.
 *
 *  Varsayilan IVT-S sonuc frame'leri (base = 0x521):
 *    base+0 I   [mA]      base+4 T  [0.1 C]
 *    base+1 U1  [mV]      base+5 W  [W]
 *    base+2 U2  [mV]      base+6 As [As]
 *    base+3 U3  [mV]      base+7 Wh [Wh]
 *    data[0] = mux, data[1] = msg count / status, data[2..5] = int32 big-endian
 *  Kendi sensor konfigurasyonuna gore id ve olcek degistir.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef IVT_CAN_H_
#define IVT_CAN_H_

#include <stdbool.h>
#include <stdint.h>

#define IVT_CAN_DATA_CURRENT    (1U << 0)
#define IVT_CAN_DATA_U1         (1U << 1)
#define IVT_CAN_DATA_U2         (1U << 2)
#define IVT_CAN_DATA_U3         (1U << 3)
#define IVT_CAN_DATA_TEMP       (1U << 4)
#define IVT_CAN_DATA_CHARGE     (1U << 5)

typedef struct {
	uint32_t	baseId;             /* 0x521                    */
	bool		invertCurrent;      /* sensor montaj yonu       */
}IVT_CAN_Cfg_t;

typedef struct {
	int32_t		currentMa;          /* pozitif = sarj           */
	int32_t		u1Mv;
	int32_t		u2Mv;
	int32_t		u3Mv;
	int16_t		tempDeciDeg;
	int64_t		chargeMas;          /* As x 1000                */
}IVT_CAN_Data_t;

typedef struct {
	void	(*onDataUpdated)(uint32_t dataMask, uint32_t tickMs);   /* IVT_CAN_DATA_* */
}IVT_CAN_EventFnMap_t;

void					IVT_CAN_Init(const IVT_CAN_Cfg_t *cfg, const IVT_CAN_EventFnMap_t *eventFnMap);
const IVT_CAN_Data_t	*IVT_CAN_GetData(void);

/* Ana dongude, alinan her frame icin. Bilinmeyen id'ler yok sayilir. */
void					IVT_CAN_OnFrame(uint32_t id, const uint8_t *data, uint8_t len, uint32_t tickMs);

#endif /* IVT_CAN_H_ */
