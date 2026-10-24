/*
 * Contactor_Connector.c
 *
 *  Kontaktor surucusu (GPIO) <-> BMS contactor connector'i.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "Contactor_Connector.h"
#include "Board.h"

#define CONTACTOR_CONN_COUNT   3U


static CONTACTOR_CONN_Cfg_t	_cfg;
static bool					_feedbackClosed[CONTACTOR_CONN_COUNT];   /* BMS'e bagli; yerinde okur */


/* BMS hook: BMS_Run icinden, kontaktor FSM komut uretince. */
static void _applyCommand(BMS_ContactorCmd_t command, void *ctx) {

	(void)ctx;

	switch (command) {

		case BMS_CONTACTOR_CMD_PRECHARGE:
			BOARD_ContactorSet(_cfg.mainPosIndex,   false);
			BOARD_ContactorSet(_cfg.mainNegIndex,   true);
			BOARD_ContactorSet(_cfg.prechargeIndex, true);
			break;

		case BMS_CONTACTOR_CMD_CLOSE_MAIN:
			BOARD_ContactorSet(_cfg.mainNegIndex,   true);
			BOARD_ContactorSet(_cfg.mainPosIndex,   true);
			BOARD_ContactorSet(_cfg.prechargeIndex, false);
			break;

		case BMS_CONTACTOR_CMD_OPEN:
		default:
			BOARD_ContactorSet(_cfg.mainPosIndex,   false);
			BOARD_ContactorSet(_cfg.prechargeIndex, false);
			BOARD_ContactorSet(_cfg.mainNegIndex,   false);
			break;
	}
}


bool CONTACTOR_CONN_Init(const CONTACTOR_CONN_Cfg_t *cfg) {

	if (cfg == NULL) {
		return false;
	}

	_cfg = *cfg;
	memset(_feedbackClosed, 0, sizeof(_feedbackClosed));

	/* BMS kontaktor sirasi: 0 = main-, 1 = precharge, 2 = main+ (BMS_Connector.h) */
	BMS_ContactorConnector_t connector = {
		.applyCommand	= _applyCommand,
		.ctx			= NULL,
		.feedbackClosed	= { &_feedbackClosed[0], &_feedbackClosed[1], &_feedbackClosed[2] },
		.numOfFeedbacks	= CONTACTOR_CONN_COUNT,
	};

	return BMS_AttachContactorConnector(cfg->packIndex, &connector) == BMS_STATUS_OK;
}


void CONTACTOR_CONN_Update(uint32_t tickMs) {

	const uint8_t boardIndex[CONTACTOR_CONN_COUNT] = { _cfg.mainNegIndex, _cfg.prechargeIndex, _cfg.mainPosIndex };

	for (uint8_t bmsIndex = 0; bmsIndex < CONTACTOR_CONN_COUNT; bmsIndex++) {

		bool closed = BOARD_ContactorGetFeedback(boardIndex[bmsIndex]);

		if (closed != _feedbackClosed[bmsIndex]) {
			_feedbackClosed[bmsIndex] = closed;
			(void)BMS_NotifyContactor(_cfg.packIndex, bmsIndex, tickMs);
		}
	}
}
