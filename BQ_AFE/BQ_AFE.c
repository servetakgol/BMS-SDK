/*
 * BQ_AFE.c
 *
 *  Created on: May 14, 2026
 *      Author: servet
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "BQ_AFE_Types.h"
#include "BQ_AFE.h"
#include "BQ_AFE_FrameBuilder.h"
#include "BQ_AFE_Connector.h"

#include "BQ79718/BQ79718.h"

static BQ_AFE_Handle_t _hBqAfe;


static const BQ_AFE_Connector_t *_connectorOf(BQ_AFE_Device_t device) {

	switch (device) {
	case BQ_AFE_DEVICE_BQ79718: return BQ79718_GetConnector();
	default:                    return NULL;
	}
}


/* Tum connector fn'leri Init'te bir kez dogrulanir; runtime'da NULL check yok. */
static bool _connectorValid(const BQ_AFE_Connector_t *con) {

	if (con == NULL)
		return false;

	return con->createMsgRequest         != NULL &&
	       con->createMsgCtrlCmd         != NULL &&
	       con->parse                    != NULL &&
	       con->createMsgStackWake1      != NULL &&
	       con->createMsgStackWake2      != NULL &&
	       con->createMsgOtpEccTestWrite != NULL &&
	       con->createMsgOtpEccTestRead  != NULL &&
	       con->createMsgDeviceAddrDir0  != NULL &&
	       con->createMsgDeviceAddrDir1  != NULL &&
	       con->createMsgSetTopStack     != NULL &&
	       con->createMsgResetFaults     != NULL &&
	       con->createMsgDevConfig1      != NULL &&
	       con->createMsgDevConfig2      != NULL &&
	       con->createMsgProtConfig      != NULL &&
	       con->createMsgAdcConfig       != NULL &&
	       con->createMsgAdcCtrl         != NULL &&
	       con->createMsgProtCtrl        != NULL;
}


static bool _ioValid(const BQ_AFE_IOFnMap_t *io) {

	if (io == NULL)
		return false;

	/* uartClearErrors opsiyonel */
	return io->uartTransmit      != NULL &&
	       io->uartReceiveToIdle != NULL &&
	       io->setUartBaudrate   != NULL &&
	       io->getMicros         != NULL;
}


static bool _attachUartReceiveToIdle(void) {

	if (_hBqAfe.ioFnMap.uartReceiveToIdle == NULL) {
		return false;
	}

	return _hBqAfe.ioFnMap.uartReceiveToIdle(_hBqAfe.rxBuffer, sizeof(_hBqAfe.rxBuffer));
}



static bool _uartTransmit(const BQ_AFE_TxFrame_t *frame) {

	if (_hBqAfe.ioFnMap.uartTransmit == NULL) {
		return false;
	}

	_hBqAfe.txFrameBuffer = *frame;

	_hBqAfe.txBusy = true;

	if (!_hBqAfe.ioFnMap.uartTransmit(_hBqAfe.txFrameBuffer.data, _hBqAfe.txFrameBuffer.size)) {
		_hBqAfe.txBusy = false;
		return false;
	}

	return true;
}


static bool _isBusy(void) {
	return (_hBqAfe.txBusy || _hBqAfe.cableDelayActive || _hBqAfe.dataReceived);
}



static void _parseData(void) {

	if (_hBqAfe.lastReceivedRxSize == 0 || _hBqAfe.connector == NULL || _hBqAfe.connector->parse == NULL) {
		return;
	}

	uint16_t rxSize = _hBqAfe.lastReceivedRxSize;
	uint16_t offset = 0;


	while (offset + BQ_AFE_RESPONSE_FRAME_SIZE <= rxSize) {

		const uint8_t *framePtr = &_hBqAfe.rxBuffer[offset];

		uint16_t totalLen = BQ_AFE_FRAME_TotalLen(framePtr, (uint16_t)(rxSize - offset));

		if (totalLen == 0) {
			break;
		}

		uint16_t dataSize = (uint16_t)(totalLen - BQ_AFE_HEADER_LEN - BQ_AFE_CRC_LEN);

		if (BQ_AFE_FRAME_Validate(framePtr, dataSize)) {

			BQ_AFE_ResponseFrame_t frame;

			if (BQ_AFE_FRAME_Decapsulate(framePtr, &frame)) {

				uint8_t devIdx = frame.devId;

				if (devIdx < BQ_AFE_MAX_DEVICE_COUNT) {

					BQ_AFE_MsgId_t msgId = _hBqAfe.connector->parse(frame.dataPtr, frame.initByte.bit.dataSize, frame.regAddr, devIdx, &_hBqAfe.data[devIdx]);

					if (msgId != BQ_AFE_MSG_NONE) {

						if (frame.regAddr == _hBqAfe.connector->dir0AddrReg) {
							_hBqAfe.detectedModuleCount++;
						}

						if (_hBqAfe.msgCallback != NULL) {
							_hBqAfe.msgCallback(devIdx, msgId);
						}
					}
				}
			}
		}

		offset += totalLen;
	}

}



bool BQ_AFE_Init(BQ_AFE_Device_t device, uint8_t numOfDeviceCount, bool useBridge, const BQ_AFE_IOFnMap_t *ioFnMap) {

	(void)numOfDeviceCount;
	(void)useBridge;

	memset(&_hBqAfe, 0, sizeof(_hBqAfe));

	const BQ_AFE_Connector_t *con = _connectorOf(device);

	if (!_connectorValid(con) || !_ioValid(ioFnMap)) {
		return false;   /* connector NULL kalir; process'ler ERROR verir */
	}

	_hBqAfe.connector    = con;
	_hBqAfe.ioFnMap      = *ioFnMap;
	_hBqAfe.cableDelayUs = BQ_AFE_CABLE_DELAY_REQUEST_US;

	return true;
}


void BQ_AFE_SetMsgCallback(BQ_AFE_MsgCallback_t callback) {

	_hBqAfe.msgCallback = callback;
}


void BQ_AFE_WakeProcess(BQ_AFE_ProcessControl_t *control) {

	BQ_AFE_TxFrame_t txFrame = {0};
	bool ok = true;

	if (*control == BQ_AFE_PROCESS_START) {
		*control = BQ_AFE_PROCESS_RUNNING;
	}

	if (*control != BQ_AFE_PROCESS_RUNNING) {
		_hBqAfe.wakeState = BQ_AFE_WAKE_STATE_BAUD_WAKE;
		return;
	}

	const BQ_AFE_Connector_t *con = _hBqAfe.connector;

	if (con == NULL) {
		*control = BQ_AFE_PROCESS_ERROR;
		return;
	}

	_hBqAfe.cableDelayUs = BQ_AFE_CABLE_DELAY_WAKE_US;

	switch (_hBqAfe.wakeState) {

	case BQ_AFE_WAKE_STATE_BAUD_WAKE:			ok = _hBqAfe.ioFnMap.setUartBaudrate(BQ_COM_WAKE_BAUD);									break;
	case BQ_AFE_WAKE_STATE_WAKE_PULSE:			txFrame.size = 1;																		break;
	case BQ_AFE_WAKE_STATE_BAUD_NORMAL:			ok = _hBqAfe.ioFnMap.setUartBaudrate(BQ_COM_NORMAL_BAUD);								break;
	case BQ_AFE_WAKE_STATE_RX_ATTACH:			ok = _attachUartReceiveToIdle();														break;
	case BQ_AFE_WAKE_STATE_SEND_WAKE_DEV_CHAIN:	ok = con->createMsgCtrlCmd(BQ_AFE_CTRL_CMD_WAKE_NEXT_CHAIN, BQ_AFE_TARGET_BROADCAST, &txFrame);	break;
	case BQ_AFE_WAKE_STATE_STACK_WAKE_1:		ok = con->createMsgStackWake1(BQ_AFE_TARGET_BROADCAST, &txFrame);								break;
	case BQ_AFE_WAKE_STATE_STACK_WAKE_2:		ok = con->createMsgStackWake2(BQ_AFE_TARGET_BROADCAST, &txFrame);								break;

	default:
		break;
	}

	if (ok && txFrame.size > 0)
		ok = _uartTransmit(&txFrame);

	if (!ok) {
		*control          = BQ_AFE_PROCESS_ERROR;
		_hBqAfe.wakeState = BQ_AFE_WAKE_STATE_BAUD_WAKE;
		return;
	}

	if (_hBqAfe.wakeState >= BQ_AFE_WAKE_STATE_STACK_WAKE_2) {
		*control          = BQ_AFE_PROCESS_DONE;
		_hBqAfe.wakeState = BQ_AFE_WAKE_STATE_BAUD_WAKE;
		return;
	}

	_hBqAfe.wakeState++;
}


void BQ_AFE_AutoAddressingProcess(BQ_AFE_ProcessControl_t *control, uint8_t *numOfDetectedModules) {

	BQ_AFE_TxFrame_t txFrame = {0};
	bool ok = true;

	if (*control == BQ_AFE_PROCESS_START) {
		*control = BQ_AFE_PROCESS_RUNNING;
	}

	if (*control != BQ_AFE_PROCESS_RUNNING) {
		_hBqAfe.addrState = BQ_AFE_ADDR_STATE_OTP_ECC_WRITE;
		_hBqAfe.addrIndex = 0;
		return;
	}

	const BQ_AFE_Connector_t *con = _hBqAfe.connector;

	if (con == NULL) {
		*control = BQ_AFE_PROCESS_ERROR;
		return;
	}

	// todo :
	// read dir1 addresses (simdilik bekliyor)
	// control detected module count == BQ_AFE_MAX_DEVICE_COUNT
	// also need to be add direction changes

	_hBqAfe.cableDelayUs = BQ_AFE_CABLE_DELAY_ADDR_US;

	switch (_hBqAfe.addrState) {
	case BQ_AFE_ADDR_STATE_OTP_ECC_WRITE:	ok = con->createMsgOtpEccTestWrite(BQ_AFE_TARGET_STACK, &txFrame);								break;
	case BQ_AFE_ADDR_STATE_ADDR_WR_ENABLE:	ok = con->createMsgCtrlCmd(BQ_AFE_CTRL_CMD_ADDR_WR, BQ_AFE_TARGET_BROADCAST, &txFrame);		break;
	case BQ_AFE_ADDR_STATE_SET_DEVICE_ADDR:	ok = con->createMsgDeviceAddrDir0(_hBqAfe.addrIndex, BQ_AFE_TARGET_BROADCAST, &txFrame);		break;
	case BQ_AFE_ADDR_STATE_SET_DIR1_ADDR:	ok = con->createMsgDeviceAddrDir1(_hBqAfe.addrIndex, BQ_AFE_TARGET_BROADCAST, &txFrame);		break;
	case BQ_AFE_ADDR_STATE_SET_TOP_STACK:	ok = con->createMsgSetTopStack((BQ_AFE_Target_t)(BQ_AFE_MAX_DEVICE_COUNT - 1U), &txFrame);		break;

	case BQ_AFE_ADDR_STATE_READ_DIR0_ADDR:
		_hBqAfe.detectedModuleCount = 0;
		ok = con->createMsgRequest(BQ_AFE_REQUEST_DIR0_ADDR, BQ_AFE_TARGET_STACK, &txFrame);
		break;

	case BQ_AFE_ADDR_STATE_OTP_ECC_READ:	ok = con->createMsgOtpEccTestRead(BQ_AFE_TARGET_STACK, &txFrame);								break;
	default:																																	break;
	}

	if (ok && txFrame.size > 0)
		ok = _uartTransmit(&txFrame);

	if (!ok) {
		*control          = BQ_AFE_PROCESS_ERROR;
		_hBqAfe.addrState = BQ_AFE_ADDR_STATE_OTP_ECC_WRITE;
		_hBqAfe.addrIndex = 0;
		return;
	}

	if (_hBqAfe.addrState == BQ_AFE_ADDR_STATE_SET_DEVICE_ADDR || _hBqAfe.addrState == BQ_AFE_ADDR_STATE_SET_DIR1_ADDR) {

		_hBqAfe.addrIndex++;

		if (_hBqAfe.addrIndex < BQ_AFE_MAX_DEVICE_COUNT) {
			return;
		}

		_hBqAfe.addrIndex = 0;
	}

	if (_hBqAfe.addrState >= BQ_AFE_ADDR_STATE_OTP_ECC_READ) {
		*control          = BQ_AFE_PROCESS_DONE;
		_hBqAfe.addrState = BQ_AFE_ADDR_STATE_OTP_ECC_WRITE;

		if (numOfDetectedModules != NULL) {
			*numOfDetectedModules = _hBqAfe.detectedModuleCount;
		}
		return;
	}

	_hBqAfe.addrState++;
}
void BQ_AFE_ConfigProcess(BQ_AFE_ProcessControl_t *control, BQ_AFE_Target_t target, const BQ_AFE_Config_t *config) {

	BQ_AFE_TxFrame_t txFrame = {0};
	bool ok = true;

	if (*control == BQ_AFE_PROCESS_START) {
		*control = BQ_AFE_PROCESS_RUNNING;
	}

	if (*control != BQ_AFE_PROCESS_RUNNING) {
		_hBqAfe.configState = BQ_AFE_CFG_STATE_RESET_FAULTS;
		return;
	}

	const BQ_AFE_Connector_t *con = _hBqAfe.connector;

	if (config == NULL || con == NULL) {
		*control = BQ_AFE_PROCESS_ERROR;
		return;
	}

	_hBqAfe.cableDelayUs = BQ_AFE_CABLE_DELAY_CONFIG_US;

	switch (_hBqAfe.configState) {
	case BQ_AFE_CFG_STATE_RESET_FAULTS:	 ok = con->createMsgResetFaults(target, &txFrame);							break;
	case BQ_AFE_CFG_STATE_DEV_CONFIG_1:	 ok = con->createMsgDevConfig1(&config->devConfig1, target, &txFrame);		break;
	case BQ_AFE_CFG_STATE_DEV_CONFIG_2:	 ok = con->createMsgDevConfig2(&config->devConfig2, target, &txFrame);		break;
	case BQ_AFE_CFG_STATE_PROT_CONFIG:	 ok = con->createMsgProtConfig(&config->protConfig, target, &txFrame);		break;
	case BQ_AFE_CFG_STATE_TSREF_EN:		 ok = con->createMsgCtrlCmd(BQ_AFE_CTRL_CMD_TSREF_EN, target, &txFrame);	break;
	case BQ_AFE_CFG_STATE_ADC_CONFIG:	 ok = con->createMsgAdcConfig(&config->adcConfig, target, &txFrame);		break;
	case BQ_AFE_CFG_STATE_ADC_CTRL:		 ok = con->createMsgAdcCtrl(&config->adcCtrl, target, &txFrame);			break;
	case BQ_AFE_CFG_STATE_PROT_CTRL:	 ok = con->createMsgProtCtrl(&config->protCtrl, target, &txFrame);			break;
	case BQ_AFE_CFG_STATE_RESET_FAULTS2: ok = con->createMsgResetFaults(target, &txFrame);							break;
	default:																										break;
	}

	if (ok && txFrame.size > 0)
		ok = _uartTransmit(&txFrame);

	if (!ok) {
		*control            = BQ_AFE_PROCESS_ERROR;
		_hBqAfe.configState = BQ_AFE_CFG_STATE_RESET_FAULTS;
		return;
	}

	if (_hBqAfe.configState >= BQ_AFE_CFG_STATE_RESET_FAULTS2) {
		*control            = BQ_AFE_PROCESS_DONE;
		_hBqAfe.configState = BQ_AFE_CFG_STATE_RESET_FAULTS;
		return;
	}

	_hBqAfe.configState++;
}

BQ_AFE_Status_t BQ_AFE_Update(void) {

	if (_hBqAfe.errorOccured) {
		_hBqAfe.errorOccured     = false;
		_hBqAfe.txBusy           = false;
		_hBqAfe.dataReceived     = false;
		_hBqAfe.cableDelayActive = false;

		if (_hBqAfe.ioFnMap.uartClearErrors != NULL) {
			_hBqAfe.ioFnMap.uartClearErrors();
		}

		_attachUartReceiveToIdle();
	}

	if (_hBqAfe.dataReceived) {
		_hBqAfe.dataReceived = false;
		_parseData();
		_attachUartReceiveToIdle();
		return BQ_AFE_STATUS_BUSY;
	}

	if (_hBqAfe.txBusy)
		return BQ_AFE_STATUS_BUSY;

	if (_hBqAfe.cableDelayActive) {

		if ((_hBqAfe.ioFnMap.getMicros() - _hBqAfe.prevMicros) < _hBqAfe.cableDelayUs)
			return BQ_AFE_STATUS_BUSY;

		_hBqAfe.cableDelayActive = false;
	}

	return  BQ_AFE_STATUS_IDLE;
}


BQ_AFE_Status_t BQ_AFE_Request(BQ_AFE_Target_t target, BQ_AFE_Request_t request) {

	BQ_AFE_TxFrame_t txFrame = {0};

	if (_isBusy())
		return BQ_AFE_STATUS_BUSY;

	if (_hBqAfe.connector == NULL)
		return BQ_AFE_STATUS_ERROR;

	_hBqAfe.cableDelayUs = BQ_AFE_CABLE_DELAY_REQUEST_US;

	if (!_hBqAfe.connector->createMsgRequest(request, target, &txFrame))
		return BQ_AFE_STATUS_ERROR;

	if (!_uartTransmit(&txFrame))
		return BQ_AFE_STATUS_ERROR;

	return BQ_AFE_STATUS_OK;
}


BQ_AFE_Status_t BQ_AFE_SetControlCommand(BQ_AFE_Target_t target, BQ_AFE_CtrlCmd cmd) {

	BQ_AFE_TxFrame_t txFrame = {0};

	if (_isBusy())
		return BQ_AFE_STATUS_BUSY;

	if (_hBqAfe.connector == NULL)
		return BQ_AFE_STATUS_ERROR;

	_hBqAfe.cableDelayUs = BQ_AFE_CABLE_DELAY_REQUEST_US;

	if (!_hBqAfe.connector->createMsgCtrlCmd(cmd, target, &txFrame))
		return BQ_AFE_STATUS_ERROR;

	if (!_uartTransmit(&txFrame))
		return BQ_AFE_STATUS_ERROR;

	return BQ_AFE_STATUS_OK;
}


const BQ_AFE_Data_t *BQ_AFE_GetData(uint8_t deviceIndex) {

	return &_hBqAfe.data[deviceIndex];
}


/* ====================================================================== */
/*  IRQ geri cagrilari — busy/received/error flag'lerinin tek yazicilari   */
/* ====================================================================== */

void BQ_AFE_HandleTxCpltIrq(void) {

	_hBqAfe.txBusy = false;

	if (_hBqAfe.ioFnMap.getMicros != NULL) {
		_hBqAfe.prevMicros       = _hBqAfe.ioFnMap.getMicros();
		_hBqAfe.cableDelayActive = true;
	}
}


void BQ_AFE_HandleRxCpltIrq(uint16_t size) {

	_hBqAfe.lastReceivedRxSize = size;
	_hBqAfe.dataReceived       = true;   /* re-attach parse sonrasi BQ_AFE_Update icinde */
}


void BQ_AFE_HandleErrorIrq(void) {

	_hBqAfe.errorOccured = true;
}
