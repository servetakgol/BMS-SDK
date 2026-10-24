/*
 * BQ_AFE.h
 *
 *  Created on: May 14, 2026
 *      Author: servet
 */

#ifndef DRIVERS_BQ_AFE_BQ_AFE_H_
#define DRIVERS_BQ_AFE_BQ_AFE_H_

#include <stdbool.h>
#include <stdint.h>
#include "BQ_AFE_Types.h"
#include "BQ_AFE_FrameBuilder.h"
#include "BQ_AFE_Connector.h"

#define BQ_AFE_MAX_DEVICE_COUNT    1U

/* Faz basina cable-delay preset'leri: TxCplt sonrasi bir sonraki islemden
 * once beklenen sure. Process fonksiyonlari kendi preset'ini handle'a yazar. */
#define BQ_AFE_CABLE_DELAY_WAKE_US       UINT32_C(100000)
#define BQ_AFE_CABLE_DELAY_ADDR_US       UINT32_C(100000)
#define BQ_AFE_CABLE_DELAY_CONFIG_US     UINT32_C(200000)
#define BQ_AFE_CABLE_DELAY_REQUEST_US    UINT32_C(200)

typedef struct {

	bool     (*uartTransmit)      (const uint8_t *data, uint16_t len);
	bool     (*uartReceiveToIdle) (uint8_t *buf, uint16_t cap);
	bool     (*setUartBaudrate)   (uint32_t baud);
	void     (*uartClearErrors)   (void);   /* opsiyonel; error recovery'de cagrilir, NULL olabilir */
	uint64_t (*getMicros)         (void);
} BQ_AFE_IOFnMap_t;


/* Parse edilen her mesaj icin BQ_AFE_Update baglamindan cagrilir (ISR degil). */
typedef void (*BQ_AFE_MsgCallback_t)(uint8_t deviceIndex, BQ_AFE_MsgId_t msgId);


typedef enum {
	BQ_AFE_STATUS_IDLE = 0,
	BQ_AFE_STATUS_OK   = BQ_AFE_STATUS_IDLE,
	BQ_AFE_STATUS_BUSY,
	BQ_AFE_STATUS_ERROR,
}BQ_AFE_Status_t;


typedef enum {
	BQ_AFE_PROCESS_IDLE = 0,
	BQ_AFE_PROCESS_START,
	BQ_AFE_PROCESS_RUNNING,
	BQ_AFE_PROCESS_DONE,
	BQ_AFE_PROCESS_ERROR,
}BQ_AFE_ProcessControl_t;


typedef enum {
	BQ_AFE_WAKE_STATE_BAUD_WAKE = 0,
	BQ_AFE_WAKE_STATE_WAKE_PULSE,
	BQ_AFE_WAKE_STATE_BAUD_NORMAL,
	BQ_AFE_WAKE_STATE_RX_ATTACH,
	BQ_AFE_WAKE_STATE_SEND_WAKE_DEV_CHAIN,
	BQ_AFE_WAKE_STATE_STACK_WAKE_1,
	BQ_AFE_WAKE_STATE_STACK_WAKE_2,
}BQ_AFE_WakeState_t;


typedef enum {
	BQ_AFE_ADDR_STATE_OTP_ECC_WRITE = 0,
	BQ_AFE_ADDR_STATE_ADDR_WR_ENABLE,
	BQ_AFE_ADDR_STATE_SET_DEVICE_ADDR,
	BQ_AFE_ADDR_STATE_SET_DIR1_ADDR,
	BQ_AFE_ADDR_STATE_SET_TOP_STACK,
	BQ_AFE_ADDR_STATE_READ_DIR0_ADDR,
	BQ_AFE_ADDR_STATE_OTP_ECC_READ,
}BQ_AFE_AddrState_t;


typedef enum {
	BQ_AFE_CFG_STATE_RESET_FAULTS = 0,
	BQ_AFE_CFG_STATE_DEV_CONFIG_1,
	BQ_AFE_CFG_STATE_DEV_CONFIG_2,
	BQ_AFE_CFG_STATE_PROT_CONFIG,
	BQ_AFE_CFG_STATE_TSREF_EN,
	BQ_AFE_CFG_STATE_ADC_CONFIG,
	BQ_AFE_CFG_STATE_ADC_CTRL,
	BQ_AFE_CFG_STATE_PROT_CTRL,
	BQ_AFE_CFG_STATE_RESET_FAULTS2,
}BQ_AFE_ConfigState_t;


typedef struct {
	BQ_AFE_DevConfig1_t    devConfig1;
	BQ_AFE_DevConfig2_t    devConfig2;
	BQ_AFE_AdcConfig_t     adcConfig;
	BQ_AFE_ProtConfigGroup_t protConfig;
	BQ_AFE_AdcCtrlGroup_t  adcCtrl;
	BQ_AFE_ProtCtrlGroup_t protCtrl;
}BQ_AFE_Config_t;


typedef struct {
	BQ_AFE_IOFnMap_t            ioFnMap;
	const BQ_AFE_Connector_t   *connector;
	BQ_AFE_MsgCallback_t        msgCallback;

	uint8_t              	rxBuffer[2048];
	uint16_t 				lastReceivedRxSize;
	BQ_AFE_TxFrame_t     	txFrameBuffer;

	volatile bool  txBusy;
	volatile bool  dataReceived;
	volatile bool  errorOccured;           /* ISR set eder; recovery BQ_AFE_Update icinde */
	volatile bool  cableDelayActive;       /* TxCplt ISR prevMicros ile baslatir; sure dolunca Update temizler */

	volatile uint64_t prevMicros;
	uint32_t          cableDelayUs;        /* aktif fazin preset'i (BQ_AFE_CABLE_DELAY_*) */

	BQ_AFE_WakeState_t   wakeState;
	BQ_AFE_AddrState_t   addrState;
	BQ_AFE_ConfigState_t configState;

	uint8_t addrIndex;
	uint8_t detectedModuleCount;

	BQ_AFE_Data_t data[BQ_AFE_MAX_DEVICE_COUNT];
}BQ_AFE_Handle_t;

// todo : gpio set reset fonksiyonlari yazilacak

/* Connector ve io fn'lerinin tamligi burada bir kez dogrulanir; false donerse
 * surucu calismaz (process'ler ERROR verir). Runtime'da tek tek NULL check yok. */
bool BQ_AFE_Init( BQ_AFE_Device_t device, uint8_t numOfDeviceCount, bool useBridge, const BQ_AFE_IOFnMap_t *ioFnMap);

void BQ_AFE_SetMsgCallback( BQ_AFE_MsgCallback_t callback );


void BQ_AFE_WakeProcess           ( BQ_AFE_ProcessControl_t *control );
void BQ_AFE_AutoAddressingProcess ( BQ_AFE_ProcessControl_t *control, uint8_t *numOfDetectedModules );
void BQ_AFE_ConfigProcess         ( BQ_AFE_ProcessControl_t *control, BQ_AFE_Target_t target, const BQ_AFE_Config_t *config );


BQ_AFE_Status_t BQ_AFE_Request( BQ_AFE_Target_t target, BQ_AFE_Request_t request );

BQ_AFE_Status_t BQ_AFE_SetControlCommand( BQ_AFE_Target_t target, BQ_AFE_CtrlCmd cmd );


const BQ_AFE_Data_t *BQ_AFE_GetData( uint8_t deviceIndex );

void BQ_AFE_HandleRxCpltIrq( uint16_t size );
void BQ_AFE_HandleTxCpltIrq( void );
void BQ_AFE_HandleErrorIrq( void );

BQ_AFE_Status_t BQ_AFE_Update( void );


#endif /* DRIVERS_BQ_AFE_BQ_AFE_H_ */
