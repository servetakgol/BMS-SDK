/*
 * BQ_AFE_Types.h
 *
 *  Created on: Jun 1, 2026
 *      Author: serve
 */

#ifndef BQ_AFE_TYPES_H_
#define BQ_AFE_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

// todo: sonra
//typedef struct {
//	bool overCurrent;
//	bool underTemp;
//	bool overTemp;
//	bool underVolt;
//	bool overVolt;
//	bool cellBalancing;
//	bool system;
//	bool power;
//	bool adc;
//	bool otpCrc;
//	bool otpData;
//	bool comF;
//	bool comTone;
//	bool comHb;
//	bool comDaisy;
//	bool comUart;
//}BQ_AFE_FaultGroup_t;


// todo : uint8_t ler anlamlandirilmali
typedef struct {

	uint8_t protectionSleepCycle;
	uint8_t twarnTreshold;
	bool cbTwarnDisabled;
	bool cbAdjacentCellDisabled;
	bool nFaultEnabled;
	bool faultToneEnabled;
	bool heartBeatEnabled;
}BQ_AFE_DevConfig1_t;

typedef struct {

	bool ptcEnabled;
	bool bbPinEnabled;
	uint8_t bbPinLoc;
	uint8_t numOfCellCount;
}BQ_AFE_DevConfig2_t;


typedef struct {
	uint16_t adcDelayUs;
}BQ_AFE_AdcConfig_t;

typedef struct {
	 uint16_t ovTresholdMv;
	 uint16_t uvTresholdMv;
	 int16_t  otTresholdDegree;
	 int16_t  utTresholdDegree;
}BQ_AFE_ProtConfigGroup_t;


typedef enum {
    BQ_AFE_ADC_CFG_LP_FILTER_10HZ    = 0b000,  /* 10 Hz   */
    BQ_AFE_ADC_CFG_LP_FILTER_20HZ    = 0b001,  /* 20 Hz   */
    BQ_AFE_ADC_CFG_LP_FILTER_39HZ    = 0b010,  /* 39 Hz   */
    BQ_AFE_ADC_CFG_LP_FILTER_80HZ    = 0b011,  /* 80 Hz   */
    BQ_AFE_ADC_CFG_LP_FILTER_166HZ   = 0b100,  /* 166 Hz  */
    BQ_AFE_ADC_CFG_LP_FILTER_360HZ   = 0b101,  /* 360 Hz  */
    BQ_AFE_ADC_CFG_LP_FILTER_899HZ   = 0b110,  /* 899 Hz  */
    BQ_AFE_ADC_CFG_LP_FILTER_RSVD    = 0b111,  /* Reserved (10 Hz) */
} BQ_AFE_AdcLpfFilter;

typedef enum {
	BQ_AFE_ADC_NOT_RUN = 0,
	BQ_AFE_ADC_CFG_SINGLE_RUN = 1,
	BQ_AFE_ADC_CFG_CONTINIOUS_RUN =2,
}BQ_AFE_AdcMode;

// todo uint8_t anlamlandirilamli
typedef struct {
	bool 					freezeEnabled;
	bool 					lpfEnabled;
	bool					startAdc;
	BQ_AFE_AdcLpfFilter 	lpfFilter;
	BQ_AFE_AdcMode 			adcMode;
}BQ_AFE_AdcCtrlGroup_t;


/* OVUV/OTUT round-robin control. mode is the 2-bit OVUV_MODE/OTUT_MODE selector
 * (datasheet OVUV_CTRL2/OTUT_CTRL2); the builder asserts the GO bit when mode is
 * non-zero and leaves the lock fields unlocked. */
typedef struct {
	uint8_t ovuvMode;   /* OVUV_MODE[1:0] */
	uint8_t otutMode;   /* OTUT_MODE[1:0] */
}BQ_AFE_ProtCtrlGroup_t;


typedef enum {
	BQ_AFE_CTRL_CMD_DIR_SEL,
	BQ_AFE_CTRL_CMD_HW_RESET,
	BQ_AFE_CTRL_CMD_WAKE_NEXT_CHAIN,
	BQ_AFE_CTRL_CMD_SLEEP_TO_ACT,
	BQ_AFE_CTRL_CMD_SHUTDOWN,
	BQ_AFE_CTRL_CMD_SLEEP,
	BQ_AFE_CTRL_CMD_SOFT_RESET,
	BQ_AFE_CTRL_CMD_ADDR_WR,
	BQ_AFE_CTRL_CMD_TSREF_EN,
	BQ_AFE_CTRL_CMD_I2C_EN
}BQ_AFE_CtrlCmd;

typedef enum {
	BQ_AFE_REQUEST_CELL_VOLTAGES = 0,
	BQ_AFE_REQUEST_GPIO_VOLTAGES,
	BQ_AFE_REQUEST_PACK_VOLTAGES,
	BQ_AFE_REQUEST_DIE_TEMP,
	BQ_AFE_REQUEST_DEV_STATUS,
	BQ_AFE_REQUEST_UNIQ_ID,
	BQ_AFE_REQUEST_FAULT_SUMMARY,
	BQ_AFE_REQUEST_DIR0_ADDR,
	BQ_AFE_REQUEST_CURRENT,
}BQ_AFE_Request_t;


/* Parse edilen response'un kimligi; connector parse'i dondurur, core
 * dispatcher ile disari verir. BQ_AFE_MSG_NONE = taninmayan/gecersiz frame. */
typedef enum {
	BQ_AFE_MSG_NONE = 0,
	BQ_AFE_MSG_CELL_VOLTAGES,
	BQ_AFE_MSG_GPIO_VOLTAGES,
	BQ_AFE_MSG_PACK_VOLTAGES,
	BQ_AFE_MSG_DIE_TEMP,
	BQ_AFE_MSG_DEV_STATUS,
	BQ_AFE_MSG_UNIQ_ID,
	BQ_AFE_MSG_FAULT_SUMMARY,
	BQ_AFE_MSG_DIR0_ADDR,
	BQ_AFE_MSG_CURRENT,
}BQ_AFE_MsgId_t;


typedef enum {
	BQ_AFE_GPIO_1_2,
	BQ_AFE_GPIO_3_4,
	BQ_AFE_GPIO_5_6,
	BQ_AFE_GPIO_7_8,
	BQ_AFE_GPIO_9_10,
	BQ_AFE_GPIO_11_12,
	BQ_AFE_GPIO_13_14,
}BQ_AFE_Gpio_t;

typedef enum {

	BQ_AFE_GPIO_HIGH_Z          		 = 0, // Disabled, high-Z
	BQ_AFE_GPIO_ADC_AND_OTUT_INPUT       = 1, // ADC and OTUT inputs
	BQ_AFE_GPIO_ADC_ONLY_INPUT           = 2, // ADC only input
	BQ_AFE_GPIO_DIGITAL_INPUT            = 3, // Digital input
	BQ_AFE_GPIO_OUTPUT_HIGH              = 4, // Output high
	BQ_AFE_GPIO_OUTPUT_LOW               = 5, // Output low
	BQ_AFE_GPIO_ADC_INPUT_PULL_UP        = 6, // ADC input + weak pull-up
	BQ_AFE_GPIO_ADC_INPUT_PULL_DOWN      = 7  // ADC input + weak pull-down
}BQ_AFE_GpioMode_t;


typedef	struct {
	uint8_t faultPwr      :1;
	uint8_t faultSys      :1;
	uint8_t faultOvuv     :1;
	uint8_t faultOtut     :1;
	uint8_t faultComm     :1;
	uint8_t faultOtp      :1;
	uint8_t faultCompAdc  :1;
	uint8_t faultProt     :1;
}BQ_AFE_FaultSummary_t;


typedef struct {
	int16_t afeDieTemp1;   /* degC */
	int16_t afeDieTemp2;   /* degC */
	int16_t afeDieTemp3;   /* degC */
}BQ_AFE_IcTemperature_t;


typedef struct {
	int32_t vcellActSumInMv;   /* todo: LSB teyit (VBAT 2 mV/LSB varsayimi) */
	int32_t batInMv;
}BQ_AFE_PackVoltages_t;

typedef struct {
	uint8_t vcellAdcRun :1;
	uint8_t gpAdcRun    :1;
	uint8_t diagAnaRun  :1;
	uint8_t ovuvRun     :1;
	uint8_t otutRun     :1;
	uint8_t custCrcDone :1;
	uint8_t factCrcDone :1;
	uint8_t diagMeasRun :1;
	uint8_t avddOn      :1;
	uint8_t diagDigRun  :1;
}BQ_AFE_DevStatus_t;

typedef struct {
	uint8_t 	partId;
	uint8_t 	tapeOutRev;
	uint64_t 	dieId;
}BQ_AFE_IdGroup_t;

typedef int16_t BQ_AFE_CellVoltages_t[32];
typedef int16_t BQ_AFE_GpioVoltages_t[16];

typedef struct {
	BQ_AFE_IdGroup_t       id;
	BQ_AFE_FaultSummary_t  faultSummary;
	BQ_AFE_DevStatus_t     devStatus;
	BQ_AFE_CellVoltages_t  cellVoltagesInMv;
	BQ_AFE_GpioVoltages_t  gpioVoltagesInMv;
	BQ_AFE_IcTemperature_t dieTempInC;
	BQ_AFE_PackVoltages_t  packVoltagesInMv;
	uint8_t                dir0Addr;
	int32_t 			   currentInMa;
}BQ_AFE_Data_t;
#endif /* BQ_AFE_TYPES_H_ */
