/*
 * BMS_UserConfig.h
 *
 *  SDK derleme zamani ust sinirlari (uygulama saglar).
 *  Bu ornek: 1 pack, 32 adet BQ79718 daisy-chain, 18 hucre / cihaz.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef BMS_USERCONFIG_H_
#define BMS_USERCONFIG_H_

#define BMS_CFG_MAX_PACKS                   1U
#define BMS_CFG_MAX_MODULES                 32U
#define BMS_CFG_MAX_CELLS_PER_MODULE        18U
#define BMS_CFG_MAX_TEMP_GROUPS_PER_MODULE  2U       /* NTC grubu + AFE die grubu       */
#define BMS_CFG_MAX_TEMP_SLOTS              (32U * 8U + 32U * 2U + 1U)   /* NTC + die + shunt */
#define BMS_CFG_MAX_CURRENT_SOURCES         2U
#define BMS_CFG_MAX_FAULT_RULES             32U
#define BMS_CFG_MAX_CONTACTORS              3U
#define BMS_CFG_ISR_QUEUE_SIZE              16U      /* BMS_NotifyCurrentFromIsr kuyrugu */

#endif /* BMS_USERCONFIG_H_ */
