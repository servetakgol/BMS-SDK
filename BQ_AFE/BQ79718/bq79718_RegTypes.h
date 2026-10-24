/*
 * bq79718_types.h
 *
 * Bit-field unions and block typedefs for Texas Instruments
 * BQ79718B-Q1 (datasheet SLUSG69, May 2025).
 *
 * Naming:
 *   BQ79718_Reg<Name>_t          - one register's bit map (sizeof == 1)
 *   BQ79718_<Name>Group_t        - contiguous register block requested /
 *                                  written as ONE frame (bottom of file)
 *
 * Bit ordering: LSB-first (B0 listed first). Matches GCC/Clang/IAR/
 * Keil ARM on little-endian targets. Static asserts at the bottom of
 * the file pin sizeof() for every type so a mis-packing compiler
 * fails to build.
 */

#ifndef BQ79718_TYPES_H
#define BQ79718_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/* GPIO_CONF<N> pair - two 3-bit GPIO mode fields plus 2 SPARE
 * bits. Shared by GPIO_CONF1..5 (which configure GPIO 1-2, 3-4,
 * 5-6, 7-8, 9-10). 'lowMode' is the odd-numbered GPIO. */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t lowMode  :3;   /* B0..B2 - odd-numbered GPIO */
		uint8_t highMode :3;   /* B3..B5 - even-numbered GPIO */
		uint8_t spare    :2;   /* B6:B7 */
	} mapped;
} BQ79718_RegGpioConf_t;

/* OC_CONF<N> - shared by OC_CONF1 and OC_CONF2. */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t occThr   :2;   /* B0:B1 */
		uint8_t ocdThr   :3;   /* B2..B4 */
		uint8_t deglitch :3;   /* B5..B7 */
	} mapped;
} BQ79718_RegOcConf_t;

/* DIRn_ADDR - shared by DIR0_ADDR and DIR1_ADDR. */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t address     :6;   /* B0..B5 */
		uint8_t dirAddrRsvd :2;   /* B6:B7 */
	} mapped;
} BQ79718_RegDirAddr_t;


/* ============================================================== */
/*  SECTION 2 - Per-register bit-field types (NVM space)          */
/* ============================================================== */

/* DEV_CONF1 (0x002) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t hbEn         :1;   /* B0 */
		uint8_t ftoneEn      :1;   /* B1 */
		uint8_t nfaultEn     :1;   /* B2 */
		uint8_t noAdjCb      :1;   /* B3 */
		uint8_t cbtwarnDis   :1;   /* B4 */
		uint8_t twarnThr     :1;   /* B5 */
		uint8_t protSlpCycle :2;   /* B6:B7 */
	} mapped;
} BQ79718_RegDevConf1_t;

/* DEV_CONF2 (0x003) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t numCell  :4;   /* B0..B3 */
		uint8_t bbPinLoc :2;   /* B4:B5 */
		uint8_t bbPinEn  :1;   /* B6 */
		uint8_t ptcEn    :1;   /* B7 */
	} mapped;
} BQ79718_RegDevConf2_t;

/* DEV_CONF3 (0x02D) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t iCbLkg        :1;   /* B0 */
		uint8_t cbAdcOffset   :2;   /* B1:B2 */
		uint8_t tsRefDly      :2;   /* B3:B4 */
		uint8_t devConf3Spare :3;   /* B5..B7 */
	} mapped;
} BQ79718_RegDevConf3_t;

/* DEV_CONF5 (0x02E) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t bbPinLoc2     :3;   /* B0..B2 */
		uint8_t bbPinEn2      :1;   /* B3 */
		uint8_t devConf5Spare :4;   /* B4..B7 */
	} mapped;
} BQ79718_RegDevConf5_t;


/* COMM_CONF (0x004) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t txHoldOff     :1;   /* B0 */
		uint8_t stkRespDly    :2;   /* B1:B2 */
		uint8_t commConfSpare :5;   /* B3..B7 */
	} mapped;
} BQ79718_RegCommConf_t;

/* OV_THRESH (0x009) - OV_THR covers the full byte. */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t ovThr :8;   /* B0..B7 */
	} mapped;
} BQ79718_RegOvThresh_t;

/* UV_THRESH (0x00A) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t uvThr         :6;   /* B0..B5 */
		uint8_t uvThreshSpare :2;   /* B6:B7 */
	} mapped;
} BQ79718_RegUvThresh_t;

/* OTUT_THRESH (0x00B) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t otThr :5;   /* B0..B4 */
		uint8_t utThr :3;   /* B5..B7 */
	} mapped;
} BQ79718_RegOtutThresh_t;

/* FAULT_MSK1 (0x00F) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t mskPwr :1;   /* B0 */
		uint8_t mskSys :1;   /* B1 */
		uint8_t mskCb  :1;   /* B2 */
		uint8_t mskOv  :1;   /* B3 */
		uint8_t mskUv  :1;   /* B4 */
		uint8_t mskOt  :1;   /* B5 */
		uint8_t mskUt  :1;   /* B6 */
		uint8_t mskOc  :1;   /* B7 */
	} mapped;
} BQ79718_RegFaultMsk1_t;

/* FAULT_MSK2 (0x010) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t mskCommUart  :1;   /* B0 */
		uint8_t mskCommDsy   :1;   /* B1 */
		uint8_t mskCommHb    :1;   /* B2 */
		uint8_t mskCommFtone :1;   /* B3 */
		uint8_t mskCommFcomm :1;   /* B4 */
		uint8_t mskOtpData   :1;   /* B5 */
		uint8_t mskOtpCrc    :1;   /* B6 */
		uint8_t mskAdc       :1;   /* B7 */
	} mapped;
} BQ79718_RegFaultMsk2_t;


/* ============================================================== */
/*  SECTION 3 - Per-register bit-field types (Control space)      */
/* ============================================================== */


/* COMM_CTRL (0x308) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t topStack     :1;   /* B0 */
		uint8_t commCtrlRsvd :7;   /* B1..B7 */
	} mapped;
} BQ79718_RegCommCtrl_t;

/* CONTROL1 (0x309) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t addrWr       :1;   /* B0 */
		uint8_t softReset    :1;   /* B1 */
		uint8_t gotoSleep    :1;   /* B2 */
		uint8_t gotoShutdown :1;   /* B3 */
		uint8_t sendSlptoAct :1;   /* B4 */
		uint8_t sendWake     :1;   /* B5 */
		uint8_t sendSdHwRst  :1;   /* B6 */
		uint8_t dirSel       :1;   /* B7 */
	} mapped;
} BQ79718_RegControl1_t;

/* CONTROL2 (0x30A) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t tsRefEn       :1;   /* B0 */
		uint8_t control2Rsvd0 :1;   /* B1 */
		uint8_t i2cEn         :1;   /* B2 */
		uint8_t progGo        :1;   /* B3 */
		uint8_t control2Rsvd1 :4;   /* B4..B7 */
	} mapped;
} BQ79718_RegControl2_t;

/* ADC_CTRL1 (0x310) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t gpDr         :1;   /* B0 */
		uint8_t adcCtrl1Rsvd :7;   /* B1..B7 */
	} mapped;
} BQ79718_RegAdcCtrl1_t;

/* ADC_CTRL2 (0x311) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t adcGo      :1;   /* B0 */
		uint8_t adcMode    :2;   /* B1:B2 */
		uint8_t lpfVcellEn :1;   /* B3 */
		uint8_t lpfVcell   :3;   /* B4..B6 */
		uint8_t freezeEn   :1;   /* B7 */
	} mapped;
} BQ79718_RegAdcCtrl2_t;


/* CB_CELL18..1_CTRL (0x320..0x331) - shared layout */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t time       :5;   /* B0..B4 - balancing time selector */
		uint8_t cbCellRsvd :3;   /* B5..B7 */
	} mapped;
} BQ79718_RegCbCellCtrl_t;

/* VCBDONE_THRESH (0x332) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t cbThr             :7;   /* B0..B6 */
		uint8_t vcbDoneThreshRsvd :1;   /* B7 */
	} mapped;
} BQ79718_RegVcbDoneThresh_t;

/* OTCB_THRESH (0x333) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t otcbThr        :4;   /* B0..B3 */
		uint8_t otcbThreshRsvd :4;   /* B4..B7 */
	} mapped;
} BQ79718_RegOtcbThresh_t;

/* OVUV_CTRL1 (0x334) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t ovuvLock      :5;   /* B0..B4 */
		uint8_t ovuvThrLock   :2;   /* B5:B6 */
		uint8_t ovuvCtrl1Rsvd :1;   /* B7 */
	} mapped;
} BQ79718_RegOvuvCtrl1_t;

/* OVUV_CTRL2 (0x335) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t ovuvGo        :1;   /* B0 */
		uint8_t ovuvMode      :2;   /* B1:B2 */
		uint8_t ovuvCtrl2Rsvd :5;   /* B3..B7 */
	} mapped;
} BQ79718_RegOvuvCtrl2_t;

/* OTUT_CTRL1 (0x336) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t otutLock      :4;   /* B0..B3 */
		uint8_t otutThrLock   :2;   /* B4:B5 */
		uint8_t otutCtrl1Rsvd :2;   /* B6:B7 */
	} mapped;
} BQ79718_RegOtutCtrl1_t;

/* OTUT_CTRL2 (0x337) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t otutGo        :1;   /* B0 */
		uint8_t otutMode      :2;   /* B1:B2 */
		uint8_t otutCtrl2Rsvd :5;   /* B3..B7 */
	} mapped;
} BQ79718_RegOtutCtrl2_t;

/* BAL_CTRL1 (0x338) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t pwm          :6;   /* B0..B5 */
		uint8_t balCtrl1Rsvd :2;   /* B6:B7 */
	} mapped;
} BQ79718_RegBalCtrl1_t;

/* BAL_CTRL2 (0x339) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t balGo        :1;   /* B0 */
		uint8_t autoBal      :1;   /* B1 */
		uint8_t balAct       :1;   /* B2 */
		uint8_t otcbEn       :1;   /* B3 */
		uint8_t fltStopEn    :1;   /* B4 */
		uint8_t cbPause      :1;   /* B5 */
		uint8_t balCtrl2Rsvd :2;   /* B6:B7 */
	} mapped;
} BQ79718_RegBalCtrl2_t;

/* BAL_CTRL3 (0x33A) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t balTimeGo    :1;   /* B0 */
		uint8_t balTimeSel   :5;   /* B1..B5 */
		uint8_t balCtrl3Rsvd :2;   /* B6:B7 */
	} mapped;
} BQ79718_RegBalCtrl3_t;

/* FAULT_RST1 (0x340) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t rstPwr :1;   /* B0 */
		uint8_t rstSys :1;   /* B1 */
		uint8_t rstCb  :1;   /* B2 */
		uint8_t rstOv  :1;   /* B3 */
		uint8_t rstUv  :1;   /* B4 */
		uint8_t rstOt  :1;   /* B5 */
		uint8_t rstUt  :1;   /* B6 */
		uint8_t rstOc  :1;   /* B7 */
	} mapped;
} BQ79718_RegFaultRst1_t;

/* FAULT_RST2 (0x341) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t rstCommUart  :1;   /* B0 */
		uint8_t rstCommDsy   :1;   /* B1 */
		uint8_t rstCommHb    :1;   /* B2 */
		uint8_t rstCommFtone :1;   /* B3 */
		uint8_t rstCommFcomm :1;   /* B4 */
		uint8_t rstOtpData   :1;   /* B5 */
		uint8_t rstOtpCrc    :1;   /* B6 */
		uint8_t rstAdc       :1;   /* B7 */
	} mapped;
} BQ79718_RegFaultRst2_t;


/* ============================================================== */
/*  SECTION 4 - STATUS REGISTERS (JUST NEED)      */
/* ============================================================== */

// 0x500 - 0x509 10 Byte contains part ID


/* BAL_STAT (0x522) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t cbDone       :1;   /* B0 */
		uint8_t abortFlt     :1;   /* B1 */
		uint8_t cbRun        :1;   /* B2 */
		uint8_t cbInPause    :1;   /* B3 */
		uint8_t otPauseDet   :1;   /* B4 */
		uint8_t nvalidCbConf :1;   /* B5 */
		uint8_t balStatRsvd  :2;   /* B6:B7 */
	} mapped;
} BQ79718_RegBalStat_t;


/* DEV_STAT1 (0x530) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t vcellAdcRun :1;   /* B0 */
		uint8_t gpAdcRun    :1;   /* B1 */
		uint8_t diagAnaRun  :1;   /* B2 */
		uint8_t ovuvRun     :1;   /* B3 */
		uint8_t otutRun     :1;   /* B4 */
		uint8_t custCrcDone :1;   /* B5 */
		uint8_t factCrcDone :1;   /* B6 */
		uint8_t diagMeasRun :1;   /* B7 */
	} mapped;
} BQ79718_RegDevStat1_t;

/* DEV_STAT2 (0x531) */
typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t avddOn       :1;   /* B0 */
		uint8_t diagDigRun   :1;   /* B1 */
		uint8_t devStat2Rsvd :6;   /* B2..B7 */
	} mapped;
} BQ79718_RegDevStat2_t;



typedef union
{
	uint8_t raw;
	struct
	{
		uint8_t faultPwr   :1;   /* B0 */
		uint8_t faultComm  :1;   /* B1 */
		uint8_t faultOtp   :1;   /* B2 */
		uint8_t faultSys   :1;   /* B3 */
		uint8_t faultOvuv  :1;   /* B4 */
		uint8_t faultOtut  :1;   /* B5 */
		uint8_t faultAdcCb :1;   /* B6 */
		uint8_t faultOc    :1;   /* B7 */
	} mapped;
} BQ79718_RegFaultSummary_t;

/* ============================================================== */
/*  SECTION 5 - REGISTER GROUPS                                   */
/*  Contiguous register blocks requested / written as ONE frame.  */
/* ============================================================== */

/* GPIO_CONF1..5 */
typedef struct {
	BQ79718_RegGpioConf_t gpio1_2_Config;
	BQ79718_RegGpioConf_t gpio3_4_Config;
	BQ79718_RegGpioConf_t gpio5_6_Config;
	BQ79718_RegGpioConf_t gpio7_8_Config;
	BQ79718_RegGpioConf_t gpio9_10_Config;
	BQ79718_RegGpioConf_t gpioX_11_Config;
}BQ79718_RegGpioConfGroup_t;

/* OV_THRESH..OTUT_THRESH (0x009..0x00B) */
typedef struct {
	BQ79718_RegOvThresh_t   ovThresh;
	BQ79718_RegUvThresh_t   uvThresh;
	BQ79718_RegOtutThresh_t otutThresh;
}BQ79718_ProtThreshGroup_t;

/* OVUV_CTRL1..OTUT_CTRL2 (0x334..0x337) */
typedef struct {
	BQ79718_RegOvuvCtrl1_t ovuvCtrl1;
	BQ79718_RegOvuvCtrl2_t ovuvCtrl2;
	BQ79718_RegOtutCtrl1_t otutCtrl1;
	BQ79718_RegOtutCtrl2_t otutCtrl2;
}BQ79718_ProtectionCtrlGroup_t;

/* PARTID..DIE_ID8 (0x500..0x509) */
typedef union {
	uint8_t raw[10];
	struct {
		uint8_t partId;
		uint8_t metalRev    :4;   /* B0..B3 */
		uint8_t allLayerRev :4;   /* B4..B7 */
		uint8_t dieId[8];
	}mapped;
}BQ79718_RegIdGroup_t;

/* DEV_STAT1..DEV_STAT2 (0x530..0x531) */
typedef struct {
	BQ79718_RegDevStat1_t devStat1;
	BQ79718_RegDevStat2_t devStat2;
}BQ79718_DevStatusGroup_t;

/* VCELL18_HI..VCELL1_LO */
typedef struct {
	int16_t vcell[18];
}BQ79718_CellVoltagesGroup_t;

/* GPIO1_HI..GPIO11_LO */
typedef struct {
	int16_t gpio[11];
}BQ79718_GpioVoltagesGroup_t;

/* VCELL_ACT_SUM_HI..BAT_LO */
typedef struct
{
	int16_t vcellActSum;
	int16_t bat;
} BQ79718_PackVoltagesGroup_t;

/* DIETEMP1_HI..DIETEMP2_LO (0x5F0..0x5F3) */
typedef struct {
	int16_t dieTemp1;
	int16_t dieTemp2;
}BQ79718_DieTempGroup_t;


#ifdef __cplusplus
}
#endif

#endif /* BQ79718_TYPES_H */
