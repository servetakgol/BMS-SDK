/*
 * BQ_AFE_Connector.h
 *
 * Device connector: the BQ_AFE core reaches every device-specific frame
 * builder / parser through this table. createMsgRequest and parse are
 * required; any other entry left NULL makes the core skip that state.
 *
 *  Created on: Jul 16, 2026
 *      Author: servet
 */

#ifndef DRIVERS_BQ_AFE_BQ_AFE_CONNECTOR_H_
#define DRIVERS_BQ_AFE_BQ_AFE_CONNECTOR_H_

#include <stdbool.h>
#include <stdint.h>

#include "BQ_AFE_Types.h"
#include "BQ_AFE_FrameBuilder.h"


/* Desteklenen device'lar — BQ_AFE_Init'te secilir, connector'a core map'ler. */
typedef enum {
	BQ_AFE_DEVICE_BQ79718 = 0,
	BQ_AFE_DEVICE_COUNT,
}BQ_AFE_Device_t;


typedef struct {

	/* runtime */
	bool           (*createMsgRequest) (BQ_AFE_Request_t request, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool           (*createMsgCtrlCmd) (BQ_AFE_CtrlCmd cmd,       BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	BQ_AFE_MsgId_t (*parse)            (const uint8_t *payload, uint16_t size, uint16_t regAddr, uint8_t deviceIndex, BQ_AFE_Data_t *dataPtr);

	/* wake */
	bool (*createMsgStackWake1) (BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgStackWake2) (BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);

	/* addressing */
	bool (*createMsgOtpEccTestWrite) (BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgOtpEccTestRead)  (BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgDeviceAddrDir0)  (uint8_t addr, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgDeviceAddrDir1)  (uint8_t addr, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgSetTopStack)     (BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);

	/* config */
	bool (*createMsgResetFaults) (BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgDevConfig1)  (const BQ_AFE_DevConfig1_t     *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgDevConfig2)  (const BQ_AFE_DevConfig2_t     *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgProtConfig)  (const BQ_AFE_ProtConfigGroup_t *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgAdcConfig)   (const BQ_AFE_AdcConfig_t      *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgAdcCtrl)     (const BQ_AFE_AdcCtrlGroup_t   *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
	bool (*createMsgProtCtrl)    (const BQ_AFE_ProtCtrlGroup_t  *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);

	/* metadata */
	uint8_t  maxCellCount;
	uint8_t  maxGpioCount;
	uint16_t dir0AddrReg;
}BQ_AFE_Connector_t;


#endif /* DRIVERS_BQ_AFE_BQ_AFE_CONNECTOR_H_ */
