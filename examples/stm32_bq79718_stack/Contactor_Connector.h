/*
 * Contactor_Connector.h
 *
 *  Kontaktor surucusu (GPIO) <-> BMS contactor connector'i.
 *
 *  BMS kontaktor FSM'i komut uretince applyCommand hook'u buradan GPIO'lara
 *  gider. Aux kontak geri bildirimi her tur okunur, degistiyse BMS'e haber
 *  verilir.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef CONTACTOR_CONNECTOR_H_
#define CONTACTOR_CONNECTOR_H_

#include <stdbool.h>
#include <stdint.h>

#include "BMS.h"

typedef struct {
	uint8_t		packIndex;
	uint8_t		mainNegIndex;         /* BOARD_ContactorSet index'leri */
	uint8_t		prechargeIndex;
	uint8_t		mainPosIndex;
}CONTACTOR_CONN_Cfg_t;

bool CONTACTOR_CONN_Init(const CONTACTOR_CONN_Cfg_t *cfg);
void CONTACTOR_CONN_Update(uint32_t tickMs);   /* feedback oku, degistiyse notify */

#endif /* CONTACTOR_CONNECTOR_H_ */
