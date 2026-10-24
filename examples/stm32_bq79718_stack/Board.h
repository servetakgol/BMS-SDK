/*
 * Board.h
 *
 *  Donanim katmani. HAL'i sadece Board_STM32G4.c gorur.
 *
 *  ISR'lar burada biter: UART callback'leri BQ_AFE handler'larina, FDCAN RX
 *  ring buffer'a, ADC DMA bir bayraga duser. Ana dongu bunlari ceker.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef BOARD_H_
#define BOARD_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint32_t	id;
	uint8_t		len;
	uint8_t		data[8];
}BOARD_CanFrame_t;

void		BOARD_Init(void);
uint32_t	BOARD_GetTickMs(void);
void		BOARD_CritEnter(void);
void		BOARD_CritExit(void);

/* BQ_AFE ioFnMap hedefleri */
bool		BOARD_UartTransmitDma(const uint8_t *data, uint16_t len);
bool		BOARD_UartReceiveToIdleDma(uint8_t *buffer, uint16_t capacity);
bool		BOARD_UartSetBaudrate(uint32_t baud);

/* FDCAN: ISR -> ring, ana dongu pop eder */
bool		BOARD_CanRxPop(BOARD_CanFrame_t *frameOut);
bool		BOARD_CanTx(uint32_t id, const uint8_t *data, uint8_t len);

/* ADC+DMA: tampon hazirsa pointer ve ornek sayisi, degilse NULL */
const uint16_t *BOARD_AdcTakeBuffer(uint16_t *numOfSamplesOut);

/* Kontaktorler */
void		BOARD_ContactorSet(uint8_t index, bool closed);
bool		BOARD_ContactorGetFeedback(uint8_t index);

/* BMS portFnMap hedefleri */
bool		BOARD_NvmRead(void *dst, uint32_t len);
bool		BOARD_NvmWrite(const void *src, uint32_t len);
void		BOARD_Log(uint8_t level, const char *msg);

void		BOARD_WdtKick(void);
void		BOARD_Logf(const char *fmt, ...);

#endif /* BOARD_H_ */
