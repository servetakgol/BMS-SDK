/*
 * bq79718.h
 *
 * BQ79718-specific frame builders and response parser, exported to the
 * BQ_AFE core via BQ79718_GetConnector().
 *
 *  Created on: Jun 1, 2026
 *      Author: servet
 */

#ifndef BQ_AFE_BQ79718_H_
#define BQ_AFE_BQ79718_H_

#include <stdbool.h>
#include <stdint.h>

#include "../BQ_AFE_Types.h"        /* BQ_AFE config groups, BQ_AFE_Data_t, BQ_AFE_CtrlCmd */
#include "../BQ_AFE_FrameBuilder.h" /* BQ_AFE_Target_t, BQ_AFE_TxFrame_t                */
#include "../BQ_AFE_Connector.h"    /* BQ_AFE_Connector_t                               */


/* ---- Raw -> engineering unit converters --------------------------------
 * VCELL/VBAT register'lari UNSIGNED'dir (two's complement degil) — makrolar
 * once uint16'ya cevirir. DIETEMP signed'dir. Parser'lar bu makrolarla
 * cevirip BQ_AFE_Data_t'ye muhendislik biriminde yazar.                    */
#define BQ79718_RAW_TO_MV_CELL(raw)      ((int32_t)((uint16_t)(raw)) / 10L)   /* 100 uV / LSB -> mV  */
#define BQ79718_RAW_TO_DEGC(raw)         ((int32_t)(raw) / 10L)               /* 0.1 degC / LSB -> C */
#define BQ79718_RAW_TO_MV_VBAT(raw)      ((int32_t)((uint16_t)(raw)) * 2L)    /* 2 mV / LSB -> mV    */


/* Connector'a erisim — extern sembol yerine getter. */
const BQ_AFE_Connector_t *BQ79718_GetConnector(void);


/* ---- Read-request builder ---------------------------------------------- */
/* Assembles ONE UART-ready request frame for `request` into *txFrameOut and
 * returns false on build failure or an unsupported request. The wire request
 * type + device address are derived from `target`. */
bool BQ79718_CreateMsgRequest(BQ_AFE_Request_t request, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);


/* Route a decapsulated response payload to the matching field of
 * BQ_AFE_Data_t by its register address. Returns the parsed message id,
 * BQ_AFE_MSG_NONE for an address this device doesn't parse. */
BQ_AFE_MsgId_t BQ79718_Parse(const uint8_t *payload, uint16_t size, uint16_t regAddr, uint8_t deviceIndex, BQ_AFE_Data_t *dataPtr);



/* ---- Config builders (one register group each) ------------------------ */
bool BQ79718_CreateMsgDevConfig1    (const BQ_AFE_DevConfig1_t    *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgDevConfig2    (const BQ_AFE_DevConfig2_t    *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgAdcConfig     (const BQ_AFE_AdcConfig_t     *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgProtConfigGroup    (const BQ_AFE_ProtConfigGroup_t *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgAdcCtrl       (const BQ_AFE_AdcCtrlGroup_t  *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);

/* OVUV_CTRL1/2 + OTUT_CTRL1/2 (0x334..0x337) in one block — starts the OV/UV and
 * OT/UT protection round-robins. */
bool BQ79718_CreateMsgProtectionCtrlGroup(const BQ_AFE_ProtCtrlGroup_t *cfg, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);


bool BQ79718_CreateMsgOtpEccTestWrite(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgOtpEccTestRead(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);


bool BQ79718_CreateMsgSetDir0DevAddr(uint8_t addr, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgSetDir1DevAddr(uint8_t addr, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgSetTopStack 	(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgSetStackDev 	(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);


bool BQ79718_CreateMsgCtrlCmd    (BQ_AFE_CtrlCmd cmd, BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgResetFaults(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);


/* Stack-wake register writes sent after the chain WAKE ping. */
bool BQ79718_CreateMsgStackWake1(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);
bool BQ79718_CreateMsgStackWake2(BQ_AFE_Target_t target, BQ_AFE_TxFrame_t *txFrameOut);


#endif /* BQ_AFE_BQ79718_H_ */
