/*
 * BMS_Connector.h
 *
 *  BMS'e veri girisi ve setpoint cikisi tek kavramla: CONNECTOR.
 *
 *  Uygulama init'te her kaynak icin bir connector kurar:
 *    - veri baglari   : uygulamanin / surucunun KENDI tamponlarina pointer
 *                       (kopya yok; BMS yerinde okur)
 *    - uygulama hook'lari : BMS'in setpoint degisince cagirdigi fonksiyonlar
 *                       (balance mask, kontaktor komutu, ...)
 *
 *  Veri geldiginde kaynak sadece BMS_Notify<Tip>() cagirir: O(1), dirty bit +
 *  tick. Hesap BMS_Run() icinde. BMS hicbir kaynagi taramaz.
 *
 *  Baglama sozlesmesi:
 *    - Pointer'lar init'ten sonra degismez (statik tampon).
 *    - Tampon, BMS_Run() ile AYNI baglamda (ana dongu) yazilir. ISR'dan yazilan
 *      tek sey int32 akim olabilir (atomik); o zaman NotifyCurrentFromIsr kullanilir.
 *    - Yazici her tutarli guncellemeden SONRA Notify cagirir.
 *    - Hook'lar BMS_Run() icinden, ana dongu baglaminda cagrilir; bloklamamali.
 *      Surucu mesgulse hook bekletir, kendi Update'inde yeniden dener.
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */

#ifndef BMS_CONNECTOR_H_
#define BMS_CONNECTOR_H_

#include <stdbool.h>
#include <stdint.h>

#include "BMS_Types.h"


/* ====================================================================== */
/*  Monitor connector: AFE / hucre izleyici. 1 cihaz = 1 modul.           */
/* ====================================================================== */

typedef struct {
	const int16_t		*values;         /* ntc != NULL: ratiometric mV; ntc == NULL: deciDeg */
	uint16_t			firstSlot;       /* pack-global sicaklik slot'u                       */
	uint8_t				numOfSlots;
	BMS_TempRole_t		role;
	const BMS_Ntc_t		*ntc;
	uint16_t			refMv;           /* NTC bolucu referansi (or. TSREF 5000 mV)          */
}BMS_TempGroupBinding_t;

typedef struct {
	/* ---- veri baglari ---- */
	const int16_t				*cellVoltagesMv;      /* [0] = hucre 1; numOfCells adet; zorunlu */
	uint8_t						numOfCells;
	const int32_t				*moduleVoltageMv;     /* NULL olabilir (BAT pini)                */
	const BMS_DeviceStatus_t	*deviceStatus;        /* NULL olabilir                           */
	BMS_TempGroupBinding_t		tempGroups[BMS_CFG_MAX_TEMP_GROUPS_PER_MODULE];
	uint8_t						numOfTempGroups;

	/* ---- BMS -> surucu hook'lari (NULL olabilir) ---- */
	void	(*applyBalanceMask)		(uint8_t moduleIndex, uint32_t cellMask, void *ctx);   /* bit i = hucre i+1 */
	void	(*applyFetEnable)		(uint8_t moduleIndex, bool chgEnabled, bool dchgEnabled, void *ctx);
	void	(*requestMeasurement)	(uint8_t moduleIndex, void *ctx);
	void	(*clearDeviceFaults)	(uint8_t moduleIndex, void *ctx);
	void	*ctx;
}BMS_MonitorConnector_t;

/* Connector struct'i gecici olabilir; BMS baglari kendi tablosuna kopyalar. */
BMS_Status_t BMS_AttachMonitorConnector(uint8_t packIndex, uint8_t moduleIndex, const BMS_MonitorConnector_t *connector);

#define BMS_MONITOR_DATA_CELLS            (1U << 0)
#define BMS_MONITOR_DATA_MODULE_VOLT      (1U << 1)
#define BMS_MONITOR_DATA_STATUS           (1U << 2)
#define BMS_MONITOR_DATA_TEMP_GROUP(n)    (1U << (8U + (n)))
#define BMS_MONITOR_DATA_ALL_TEMP_GROUPS  (0xFFU << 8)

/* flags: BMS_FEED_FLAG_* (ornegin dengeleme acikken alinan hucre olcumu) */
BMS_Status_t BMS_NotifyMonitor(uint8_t packIndex, uint8_t moduleIndex, uint32_t dataMask, uint32_t tickMs, uint32_t flags);


/* ====================================================================== */
/*  Current connector: akim sensoru (IVT CAN, shunt ADC, Hall...).        */
/*  Kaynagin ne oldugu BMS'i ilgilendirmez; uygulama baglar.              */
/* ====================================================================== */

typedef struct {
	const int32_t	*currentMa;            /* zorunlu; pozitif = sarj                      */
	const int64_t	*chargeMas;            /* NULL: BMS kendisi entegre eder               */
	const int32_t	*packVoltageMv;        /* NULL olabilir                                */
	const int32_t	*busVoltageMv;         /* NULL olabilir                                */
	const int16_t	*shuntTempDeciDeg;     /* NULL olabilir                                */
	uint16_t		shuntTempSlot;         /* shuntTempDeciDeg icin pack-global slot       */
}BMS_CurrentConnector_t;

BMS_Status_t BMS_AttachCurrentConnector(uint8_t packIndex, BMS_CurrentSource_t source, const BMS_CurrentConnector_t *connector);

#define BMS_CURRENT_DATA_CURRENT      (1U << 0)
#define BMS_CURRENT_DATA_CHARGE       (1U << 1)
#define BMS_CURRENT_DATA_PACK_VOLT    (1U << 2)
#define BMS_CURRENT_DATA_BUS_VOLT     (1U << 3)
#define BMS_CURRENT_DATA_SHUNT_TEMP   (1U << 4)

BMS_Status_t BMS_NotifyCurrent		(uint8_t packIndex, BMS_CurrentSource_t source, uint32_t dataMask, uint32_t tickMs);
BMS_Status_t BMS_NotifyCurrentFromIsr(uint8_t packIndex, BMS_CurrentSource_t source, uint32_t dataMask, uint32_t tickMs);


/* ====================================================================== */
/*  Contactor connector: kontaktor surucusu.                              */
/*  Index sirasi: 0 = main-, 1 = precharge, 2 = main+                     */
/* ====================================================================== */

typedef struct {
	/* ---- BMS -> surucu ---- */
	void		(*applyCommand)(BMS_ContactorCmd_t command, void *ctx);   /* zorunlu (modul aciksa) */
	void		*ctx;

	/* ---- veri baglari ---- */
	const bool	*feedbackClosed[BMS_CFG_MAX_CONTACTORS];                 /* NULL olabilir          */
	uint8_t		numOfFeedbacks;
}BMS_ContactorConnector_t;

BMS_Status_t BMS_AttachContactorConnector(uint8_t packIndex, const BMS_ContactorConnector_t *connector);
BMS_Status_t BMS_NotifyContactor(uint8_t packIndex, uint8_t contactorIndex, uint32_t tickMs);


#endif /* BMS_CONNECTOR_H_ */
