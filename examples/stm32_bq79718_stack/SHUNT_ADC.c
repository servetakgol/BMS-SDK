/*
 * SHUNT_ADC.c
 *
 *  ADC+DMA shunt surucusu.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

#include "SHUNT_ADC.h"
#include "Board.h"


typedef struct {
	SHUNT_ADC_Cfg_t			cfg;
	SHUNT_ADC_EventFnMap_t	eventFnMap;
	SHUNT_ADC_Data_t		data;
}_Handle_t;

static _Handle_t _hShunt;


void SHUNT_ADC_Init(const SHUNT_ADC_Cfg_t *cfg, const SHUNT_ADC_EventFnMap_t *eventFnMap) {

	memset(&_hShunt, 0, sizeof(_hShunt));

	_hShunt.cfg = *cfg;

	if (eventFnMap != NULL) {
		_hShunt.eventFnMap = *eventFnMap;
	}
}


const SHUNT_ADC_Data_t *SHUNT_ADC_GetData(void) {

	return &_hShunt.data;
}


void SHUNT_ADC_Update(uint32_t tickMs) {

	uint16_t numOfSamples = 0;
	const uint16_t *buffer = BOARD_AdcTakeBuffer(&numOfSamples);   /* DMA tamamlanmadiysa NULL */

	if (buffer == NULL || numOfSamples == 0U) {
		return;
	}

	uint32_t sum = 0;

	for (uint16_t index = 0; index < numOfSamples; index++) {
		sum += buffer[index];
	}

	/* Hesap float, sonuc tam sayi. */
	float avgCounts  = (float)sum / (float)numOfSamples;
	float delta      = avgCounts - (float)_hShunt.cfg.offsetCounts;
	float currentMaF = delta * ((float)_hShunt.cfg.uaPerCount / 1000.0f);

	if (_hShunt.cfg.invert) {
		currentMaF = -currentMaF;
	}

	_hShunt.data.currentMa = (int32_t)lroundf(currentMaF);

	if (_hShunt.eventFnMap.onDataUpdated != NULL) {
		_hShunt.eventFnMap.onDataUpdated(tickMs);
	}
}
