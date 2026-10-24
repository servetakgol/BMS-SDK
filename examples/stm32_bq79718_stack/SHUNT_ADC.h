/*
 * SHUNT_ADC.h
 *
 *  MCU ADC + DMA ile shunt (harici amp, or. INA240) surucusu.
 *
 *  BQ_AFE ile ayni desen: veri kendi tamponunda, guncellenince
 *  eventFnMap.onDataUpdated. BMS'i bilmez; Current_Connector baglar.
 *  Hesap float, sonuc int32.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef SHUNT_ADC_H_
#define SHUNT_ADC_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	int32_t		offsetCounts;       /* sifir akimda ADC degeri (kalibre)     */
	int32_t		uaPerCount;         /* uA / ADC LSB (shunt, amp, Vref'ten)   */
	bool		invert;
}SHUNT_ADC_Cfg_t;

typedef struct {
	int32_t		currentMa;          /* pozitif = sarj                        */
}SHUNT_ADC_Data_t;

typedef struct {
	void	(*onDataUpdated)(uint32_t tickMs);
}SHUNT_ADC_EventFnMap_t;

void					SHUNT_ADC_Init(const SHUNT_ADC_Cfg_t *cfg, const SHUNT_ADC_EventFnMap_t *eventFnMap);
const SHUNT_ADC_Data_t	*SHUNT_ADC_GetData(void);
void					SHUNT_ADC_Update(uint32_t tickMs);   /* DMA tamponu hazirsa isler */

#endif /* SHUNT_ADC_H_ */
