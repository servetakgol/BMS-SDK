/*
 * App_Config.h
 *
 *  Bu ornegin topolojisi ve SDK konfigurasyonu.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include "BMS.h"

#define APP_PACK_INDEX              0U
#define APP_STACK_DEVICE_COUNT      32U     /* daisy-chain'deki BQ79718 sayisi      */
#define APP_CELLS_PER_DEVICE        18U
#define APP_NTC_GPIO_FIRST          0U      /* GPIO1 (0 tabanli)                    */
#define APP_NTC_PER_DEVICE          8U      /* GPIO1..GPIO8 termistor               */
#define APP_DIE_TEMP_PER_DEVICE     2U
#define APP_TSREF_MV                5000U

/* Sicaklik slot yerlesimi (pack icinde global):
 *   [0 .. 255]   NTC : dev*8 + gpio
 *   [256 .. 319] die : 256 + dev*2 + k
 *   [320]        shunt (IVT)
 */
#define APP_SLOT_NTC_BASE           0U
#define APP_SLOT_DIE_BASE           (APP_STACK_DEVICE_COUNT * APP_NTC_PER_DEVICE)
#define APP_SLOT_SHUNT              (APP_SLOT_DIE_BASE + APP_STACK_DEVICE_COUNT * APP_DIE_TEMP_PER_DEVICE)
#define APP_TEMP_SLOT_COUNT         (APP_SLOT_SHUNT + 1U)

/* Kontaktor index'leri (BOARD_ContactorSet) */
#define APP_CONTACTOR_MAIN_NEG      0U
#define APP_CONTACTOR_PRECHARGE     1U
#define APP_CONTACTOR_MAIN_POS      2U

extern const BMS_SystemCfg_t	appSystemCfgG;
extern const BMS_Ntc_t			appNtc10kB3435G;

#endif /* APP_CONFIG_H_ */
