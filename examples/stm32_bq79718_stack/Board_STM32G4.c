/*
 * Board_STM32G4.c
 *
 *  STM32G4 HAL uzerinde Board katmani.
 *
 *  CubeMX'ten gelen handle'lar: huart1 (BQ79600 bridge), hfdcan1 (IVT +
 *  telemetri), hadc1 + DMA (shunt amp), hiwdg. Pin isimleri ornektir.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "stm32g4xx_hal.h"
#include "Board.h"
#include "BQ_AFE.h"

extern UART_HandleTypeDef	huart1;
extern FDCAN_HandleTypeDef	hfdcan1;
extern ADC_HandleTypeDef	hadc1;
extern IWDG_HandleTypeDef	hiwdg;


/* ---- FDCAN RX ring (ISR yazar, ana dongu okur) ------------------------- */

#define BOARD_CAN_RING_SIZE   32U   /* 2'nin kuvveti */

static BOARD_CanFrame_t		_canRing[BOARD_CAN_RING_SIZE];
static volatile uint16_t	_canHead;   /* ISR  */
static volatile uint16_t	_canTail;   /* main */

/* ---- ADC DMA tamponu ---------------------------------------------------- */

#define BOARD_ADC_SAMPLES     64U

static uint16_t				_adcBuffer[BOARD_ADC_SAMPLES];
static volatile bool		_adcReady;

/* ---- Kontaktor pinleri -------------------------------------------------- */

typedef struct {
	GPIO_TypeDef	*port;
	uint16_t		pin;
}_Pin_t;

static const _Pin_t _contactorDrive[3] = {
	{ GPIOB, GPIO_PIN_0 },   /* main-     */
	{ GPIOB, GPIO_PIN_1 },   /* precharge */
	{ GPIOB, GPIO_PIN_2 },   /* main+     */
};

static const _Pin_t _contactorFeedback[3] = {
	{ GPIOB, GPIO_PIN_3 },
	{ GPIOB, GPIO_PIN_4 },
	{ GPIOB, GPIO_PIN_5 },
};

/* ---- Kritik bolge (PRIMASK, ic ice dayanikli) --------------------------- */

static uint32_t	_primask;
static uint8_t	_critDepth;


/* ====================================================================== */
/*  Init / zaman / kritik bolge                                           */
/* ====================================================================== */

void BOARD_Init(void) {

	/* HAL_Init, SystemClock_Config, MX_*_Init CubeMX main.c'de cagrilmis kabul
	 * edilir; burada sadece surekli calisan seyler baslatilir. */
	_canHead  = 0;
	_canTail  = 0;
	_adcReady = false;
	_critDepth = 0;

	(void)HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
	(void)HAL_FDCAN_Start(&hfdcan1);
	(void)HAL_ADC_Start_DMA(&hadc1, (uint32_t *)_adcBuffer, BOARD_ADC_SAMPLES);   /* circular */
}

uint32_t BOARD_GetTickMs(void) {

	return HAL_GetTick();
}

void BOARD_CritEnter(void) {

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	if (_critDepth++ == 0U) {
		_primask = primask;
	}
}

void BOARD_CritExit(void) {

	if (_critDepth > 0U && --_critDepth == 0U) {
		__set_PRIMASK(_primask);
	}
}


/* ====================================================================== */
/*  UART (BQ_AFE ioFnMap)                                                 */
/* ====================================================================== */

bool BOARD_UartTransmitDma(const uint8_t *data, uint16_t len) {

	return HAL_UART_Transmit_DMA(&huart1, data, len) == HAL_OK;
}

bool BOARD_UartReceiveToIdleDma(uint8_t *buffer, uint16_t capacity) {

	return HAL_UARTEx_ReceiveToIdle_DMA(&huart1, buffer, capacity) == HAL_OK;
}

bool BOARD_UartSetBaudrate(uint32_t baud) {

	/* Wake icin 2900 baud, sonra 1 Mbaud. Yeniden init DMA'yi keser; surucu
	 * bunu sadece init FSM'inde, RX kurulmadan once cagirir. */
	(void)HAL_UART_DeInit(&huart1);
	huart1.Init.BaudRate = baud;

	return HAL_UART_Init(&huart1) == HAL_OK;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {

	if (huart == &huart1) {
		BQ_AFE_HandleTxCpltIrq();
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {

	if (huart == &huart1) {
		BQ_AFE_HandleRxCpltIrq(size);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {

	if (huart == &huart1) {
		BQ_AFE_HandleErrorIrq();
	}
}


/* ====================================================================== */
/*  FDCAN                                                                 */
/* ====================================================================== */

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {

	if (hfdcan != &hfdcan1 || (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U) {
		return;
	}

	FDCAN_RxHeaderTypeDef header;
	uint8_t data[8];

	while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0U) {

		if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &header, data) != HAL_OK) {
			break;
		}

		uint16_t next = (uint16_t)((_canHead + 1U) & (BOARD_CAN_RING_SIZE - 1U));

		if (next == _canTail) {
			break;   /* ring dolu: frame duser, BMS stale ile yakalar */
		}

		BOARD_CanFrame_t *frame = &_canRing[_canHead];
		frame->id = header.Identifier;

		/* HAL surumune gore DataLength ham DLC veya <<16 kodlu olabilir. */
		uint32_t dlc = header.DataLength;
		if (dlc > 15U) {
			dlc >>= 16;
		}
		frame->len = (uint8_t)((dlc > 8U) ? 8U : dlc);
		memcpy(frame->data, data, frame->len);

		_canHead = next;
	}
}

bool BOARD_CanRxPop(BOARD_CanFrame_t *frameOut) {

	if (_canTail == _canHead) {
		return false;
	}

	*frameOut = _canRing[_canTail];
	_canTail  = (uint16_t)((_canTail + 1U) & (BOARD_CAN_RING_SIZE - 1U));

	return true;
}

bool BOARD_CanTx(uint32_t id, const uint8_t *data, uint8_t len) {

	FDCAN_TxHeaderTypeDef header;
	memset(&header, 0, sizeof(header));

	header.Identifier          = id;
	header.IdType              = FDCAN_STANDARD_ID;
	header.TxFrameType         = FDCAN_DATA_FRAME;
	header.DataLength          = (uint32_t)len;   /* HAL surumune gore FDCAN_DLC_BYTES_n */
	header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	header.BitRateSwitch       = FDCAN_BRS_OFF;
	header.FDFormat            = FDCAN_CLASSIC_CAN;
	header.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;

	return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &header, (uint8_t *)data) == HAL_OK;
}


/* ====================================================================== */
/*  ADC                                                                   */
/* ====================================================================== */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {

	if (hadc == &hadc1) {
		_adcReady = true;
	}
}

const uint16_t *BOARD_AdcTakeBuffer(uint16_t *numOfSamplesOut) {

	if (!_adcReady) {
		return NULL;
	}

	_adcReady = false;
	*numOfSamplesOut = BOARD_ADC_SAMPLES;

	return _adcBuffer;   /* circular DMA: okuma, bir sonraki dolumdan once biter */
}


/* ====================================================================== */
/*  Kontaktor                                                             */
/* ====================================================================== */

void BOARD_ContactorSet(uint8_t index, bool closed) {

	if (index < 3U) {
		HAL_GPIO_WritePin(_contactorDrive[index].port, _contactorDrive[index].pin,
		                  closed ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}
}

bool BOARD_ContactorGetFeedback(uint8_t index) {

	if (index >= 3U) {
		return false;
	}

	return HAL_GPIO_ReadPin(_contactorFeedback[index].port, _contactorFeedback[index].pin) == GPIO_PIN_SET;
}


/* ====================================================================== */
/*  NVM / log / watchdog                                                  */
/* ====================================================================== */

bool BOARD_NvmRead(void *dst, uint32_t len) {

	/* Uygulamaya gore: dahili flash sayfasi (cift kopya), EEPROM emulasyonu
	 * veya harici FRAM. Bu ornekte yer tutucu. */
	(void)dst; (void)len;
	return false;
}

bool BOARD_NvmWrite(const void *src, uint32_t len) {

	(void)src; (void)len;
	return false;
}

void BOARD_Log(uint8_t level, const char *msg) {

	(void)printf("[bms:%u] %s\n", (unsigned)level, msg);   /* printf retarget (SWO/UART) varsayilir */
}

void BOARD_Logf(const char *fmt, ...) {

	char line[128];
	va_list args;

	va_start(args, fmt);
	(void)vsnprintf(line, sizeof(line), fmt, args);
	va_end(args);

	(void)puts(line);
}

void BOARD_WdtKick(void) {

	(void)HAL_IWDG_Refresh(&hiwdg);
}
