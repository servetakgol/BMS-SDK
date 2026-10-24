/*
 * Current_Connector.c
 *
 *  Akim kaynaklari <-> BMS current connector'lari.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "Current_Connector.h"
#include "IVT_CAN.h"
#include "SHUNT_ADC.h"


static CURRENT_CONN_Cfg_t _cfg;


bool CURRENT_CONN_Init(const CURRENT_CONN_Cfg_t *cfg) {

	if (cfg == NULL) {
		return false;
	}

	_cfg = *cfg;

	/* Birincil: IVT. Akim, (opsiyonel) As sayaci, pack/bus voltaji, shunt sicakligi. */
	const IVT_CAN_Data_t *ivt = IVT_CAN_GetData();

	BMS_CurrentConnector_t primary = {
		.currentMa			= &ivt->currentMa,
		.chargeMas			= cfg->ivtUseChargeCounter ? &ivt->chargeMas : NULL,
		.packVoltageMv		= cfg->ivtU1IsPack ? &ivt->u1Mv : NULL,
		.busVoltageMv		= cfg->ivtU2IsBus  ? &ivt->u2Mv : NULL,
		.shuntTempDeciDeg	= &ivt->tempDeciDeg,
		.shuntTempSlot		= cfg->shuntTempSlot,
	};

	if (BMS_AttachCurrentConnector(cfg->packIndex, BMS_CURRENT_SRC_PRIMARY, &primary) != BMS_STATUS_OK) {
		return false;
	}

	/* Ikincil: shunt ADC. Sadece akim; BMS iki kaynagi plausibility icin karsilastirir. */
	const SHUNT_ADC_Data_t *shunt = SHUNT_ADC_GetData();

	BMS_CurrentConnector_t secondary = {
		.currentMa			= &shunt->currentMa,
		.chargeMas			= NULL,
		.packVoltageMv		= NULL,
		.busVoltageMv		= NULL,
		.shuntTempDeciDeg	= NULL,
		.shuntTempSlot		= 0,
	};

	if (BMS_AttachCurrentConnector(cfg->packIndex, BMS_CURRENT_SRC_SECONDARY, &secondary) != BMS_STATUS_OK) {
		return false;
	}

	return true;
}


void CURRENT_CONN_OnIvtDataUpdated(uint32_t ivtDataMask, uint32_t tickMs) {

	uint32_t bmsDataMask = 0U;

	if (ivtDataMask & IVT_CAN_DATA_CURRENT)                        bmsDataMask |= BMS_CURRENT_DATA_CURRENT;
	if ((ivtDataMask & IVT_CAN_DATA_CHARGE) && _cfg.ivtUseChargeCounter) bmsDataMask |= BMS_CURRENT_DATA_CHARGE;
	if ((ivtDataMask & IVT_CAN_DATA_U1) && _cfg.ivtU1IsPack)        bmsDataMask |= BMS_CURRENT_DATA_PACK_VOLT;
	if ((ivtDataMask & IVT_CAN_DATA_U2) && _cfg.ivtU2IsBus)         bmsDataMask |= BMS_CURRENT_DATA_BUS_VOLT;
	if (ivtDataMask & IVT_CAN_DATA_TEMP)                           bmsDataMask |= BMS_CURRENT_DATA_SHUNT_TEMP;

	if (bmsDataMask != 0U) {
		(void)BMS_NotifyCurrent(_cfg.packIndex, BMS_CURRENT_SRC_PRIMARY, bmsDataMask, tickMs);
	}
}


void CURRENT_CONN_OnShuntDataUpdated(uint32_t tickMs) {

	(void)BMS_NotifyCurrent(_cfg.packIndex, BMS_CURRENT_SRC_SECONDARY, BMS_CURRENT_DATA_CURRENT, tickMs);
}
