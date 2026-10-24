/*
 * BMS_Types.h
 *
 *  Temel tipler, birimler, bayraklar. API sinirinda float yoktur; her deger
 *  tam sayi birimde tasinir (mV, mA, mAs, deciDeg = 0.1 C, ms, 0.01 %).
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef BMS_TYPES_H_
#define BMS_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

#include "BMS_UserConfig.h"   /* uygulama saglar: BMS_CFG_* ust sinirlari */


#define BMS_VOLTAGE_INVALID_MV      ((int16_t)INT16_MIN)
#define BMS_TEMP_INVALID_DECIDEG    ((int16_t)INT16_MIN)


typedef enum {
	BMS_STATUS_OK = 0,
	BMS_STATUS_BUSY,
	BMS_STATUS_ERROR,
	BMS_STATUS_INVALID_PARAM,
	BMS_STATUS_NOT_ATTACHED,
}BMS_Status_t;


/* ---- Roller ve kaynaklar ---------------------------------------------- */

typedef enum {
	BMS_TEMP_ROLE_CELL = 0,     /* hucre / modul yuzeyi NTC: fault ve limit hesabina girer */
	BMS_TEMP_ROLE_MODULE,
	BMS_TEMP_ROLE_BUSBAR,
	BMS_TEMP_ROLE_SHUNT,        /* akim sensoru sicakligi                                   */
	BMS_TEMP_ROLE_AMBIENT,
	BMS_TEMP_ROLE_AFE_DIE,      /* AFE die: balancing termal kisiti                         */
	BMS_TEMP_ROLE_COUNT
}BMS_TempRole_t;

typedef enum {
	BMS_CURRENT_SRC_PRIMARY = 0,
	BMS_CURRENT_SRC_SECONDARY,
}BMS_CurrentSource_t;


/* ---- Notify / Feed bayraklari ----------------------------------------- */

#define BMS_FEED_FLAG_BALANCING_ACTIVE   (1U << 0)   /* ornek dengeleme etkisi altinda alindi  */
#define BMS_FEED_FLAG_PARTIAL            (1U << 1)   /* dizi eksik, sadece ilk n gecerli       */
#define BMS_FEED_FLAG_SUSPECT            (1U << 2)   /* kaynak guvenilir degil (CRC retry vb.) */


/* ---- AFE cihaz durumu (BMS_FeedDeviceStatus) -------------------------- */

typedef union {
	uint16_t raw;
	struct {
		uint16_t overVolt     :1;
		uint16_t underVolt    :1;
		uint16_t overTemp     :1;
		uint16_t underTemp    :1;
		uint16_t overCurrent  :1;
		uint16_t comm         :1;
		uint16_t system       :1;
		uint16_t power        :1;
		uint16_t otp          :1;
		uint16_t adc          :1;
		uint16_t reserved     :6;
	} mapped;
}BMS_DeviceFaultFlags_t;

typedef struct {
	BMS_DeviceFaultFlags_t	faultFlags;        /* AFE'nin kendi koruma / hata bitleri */
	uint16_t				commErrorCount;    /* kumulatif                           */
	bool					online;            /* cihaz son dongude cevap verdi       */
}BMS_DeviceStatus_t;


/* ---- Seviye, mod, kontaktor ------------------------------------------- */

typedef enum {
	BMS_SEVERITY_NONE = 0,
	BMS_SEVERITY_INFO,
	BMS_SEVERITY_WARN,
	BMS_SEVERITY_DERATE,
	BMS_SEVERITY_TRIP,
}BMS_Severity_t;

typedef enum {
	BMS_MODE_STANDBY = 0,
	BMS_MODE_ACTIVE,
	BMS_MODE_CHARGE,
	BMS_MODE_SLEEP,
}BMS_Mode_t;

typedef enum {
	BMS_CONTACTOR_CMD_OPEN = 0,
	BMS_CONTACTOR_CMD_PRECHARGE,
	BMS_CONTACTOR_CMD_CLOSE_MAIN,
}BMS_ContactorCmd_t;

typedef enum {
	BMS_CONTACTOR_STATE_OPEN = 0,
	BMS_CONTACTOR_STATE_PRECHARGE,
	BMS_CONTACTOR_STATE_CLOSED,
	BMS_CONTACTOR_STATE_FAULT_OPEN,
}BMS_ContactorState_t;

typedef enum {
	BMS_CONTACTOR_REQ_OPEN = 0,
	BMS_CONTACTOR_REQ_CLOSE,
}BMS_ContactorReq_t;


/* ---- NTC tanimi (ratiometric donusum) --------------------------------- */

typedef struct {
	uint32_t	r25Ohm;        /* 25 C direnci                           */
	uint16_t	betaK;         /* B25/85                                 */
	uint32_t	rSeriesOhm;    /* seri direnc (pull-up veya pull-down)   */
	bool		ntcToGnd;      /* true: NTC GND'ye, seri direnc referansa*/
}BMS_Ntc_t;


/* ---- Port fonksiyon haritasi (BQ79718_IOFnMap_t ile ayni desen) ------- */

typedef struct {
	uint32_t	(*getTickMs) (void);
	void		(*critEnter) (void);                              /* FromIsr kuyrugu icin */
	void		(*critExit)  (void);
	bool		(*nvmRead)   (void *dst, uint32_t len);           /* NULL olabilir        */
	bool		(*nvmWrite)  (const void *src, uint32_t len);     /* NULL olabilir        */
	void		(*log)       (uint8_t level, const char *msg);    /* NULL olabilir        */
}BMS_PortFnMap_t;


#endif /* BMS_TYPES_H_ */
