/*
 * bq79718_addr.h
 *
 * Customer-accessible register addresses and block-size constants
 * for Texas Instruments BQ79718B-Q1 (datasheet SLUSG69, May 2025).
 *
 * Diagnostic-only registers and the DEBUG block are intentionally
 * omitted (DIAG_*, FAULT_ADC_*, FAULT_CB_FETOW*, DIAG_MAIN/RDNT,
 * DIAG_D1/D2, REF_CAP_T0, OTP_ECC_TEST, all DEBUG_*).
 *
 * Block reads start at the first ADDR of the group and run for
 * <block>_SIZE bytes; the matching block typedef in
 * bq79718_types.h declares the byte layout.
 */

#ifndef BQ79718_ADDR_H
#define BQ79718_ADDR_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- NVM Backed Registers (0x000-0x02F) ---- */
//#define BQ79718_ADDR_CUST_CRC_HI         0x000U
//#define BQ79718_ADDR_CUST_CRC_LO         0x001U
#define BQ79718_ADDR_DEV_CONF1           0x002U
#define BQ79718_ADDR_DEV_CONF2           0x003U
#define BQ79718_ADDR_COMM_CONF           0x004U
//#define BQ79718_ADDR_BBVC_POSN1          0x005U
//#define BQ79718_ADDR_BBVC_POSN2          0x006U
//#define BQ79718_ADDR_BBVC_POSN3          0x007U
#define BQ79718_ADDR_ADC_CONF            0x008U
#define BQ79718_ADDR_OV_THRESH           0x009U
#define BQ79718_ADDR_UV_THRESH           0x00AU
#define BQ79718_ADDR_OTUT_THRESH         0x00BU
//#define BQ79718_ADDR_UV_DISABLE1         0x00CU
//#define BQ79718_ADDR_UV_DISABLE2         0x00DU
//#define BQ79718_ADDR_UV_DISABLE3         0x00EU
#define BQ79718_ADDR_FAULT_MSK1          0x00FU
#define BQ79718_ADDR_FAULT_MSK2          0x010U
//#define BQ79718_ADDR_CS_ADC_CAL1         0x011U
//#define BQ79718_ADDR_CS_ADC_CAL2         0x012U
//#define BQ79718_ADDR_OC_CONF1            0x013U
//#define BQ79718_ADDR_OC_CONF2            0x014U
//#define BQ79718_ADDR_CUST_MISC1          0x015U
//#define BQ79718_ADDR_CUST_MISC2          0x016U
//#define BQ79718_ADDR_CUST_MISC3          0x017U
//#define BQ79718_ADDR_CUST_MISC4          0x018U
//#define BQ79718_ADDR_CUST_MISC5          0x019U
//#define BQ79718_ADDR_CUST_MISC6          0x01AU
//#define BQ79718_ADDR_CUST_MISC7          0x01BU
//#define BQ79718_ADDR_CUST_MISC8          0x01CU
//#define BQ79718_ADDR_IDDQ_CONF           0x01DU
#define BQ79718_ADDR_GPIO_CONF1          0x01EU
#define BQ79718_ADDR_GPIO_CONF2          0x01FU
#define BQ79718_ADDR_GPIO_CONF3          0x020U
#define BQ79718_ADDR_GPIO_CONF4          0x021U
#define BQ79718_ADDR_GPIO_CONF5          0x022U
#define BQ79718_ADDR_GPIO_CONF6          0x023U
//#define BQ79718_ADDR_VCELL_OFFSET1       0x024U
//#define BQ79718_ADDR_VCELL_OFFSET2       0x025U
//#define BQ79718_ADDR_VCELL_OFFSET3       0x026U
//#define BQ79718_ADDR_VCELL_OFFSET4       0x027U
//#define BQ79718_ADDR_VCELL_OFFSET5       0x028U
//#define BQ79718_ADDR_VCELL_OFFSET6       0x029U
//#define BQ79718_ADDR_VCELL_OFFSET7       0x02AU
//#define BQ79718_ADDR_VCELL_OFFSET8       0x02BU
//#define BQ79718_ADDR_VCELL_OFFSET9       0x02CU
//#define BQ79718_ADDR_DEV_CONF3           0x02DU
//#define BQ79718_ADDR_DEV_CONF5           0x02EU
//#define BQ79718_ADDR_OTP_SPARE1          0x02FU

/* ---- Control Registers (0x300-0x4FF) ---- */
//#define BQ79718_ADDR_OTP_PROG_UNLOCK1    0x300U
#define BQ79718_ADDR_DIR0_ADDR           0x306U
#define BQ79718_ADDR_DIR1_ADDR           0x307U
#define BQ79718_ADDR_COMM_CTRL           0x308U
#define BQ79718_ADDR_CONTROL1            0x309U
#define BQ79718_ADDR_CONTROL2            0x30AU
#define BQ79718_ADDR_ADC_CTRL1           0x310U
#define BQ79718_ADDR_ADC_CTRL2           0x311U
#define BQ79718_ADDR_ADC_CTRL3           0x312U
#define BQ79718_ADDR_ADC_CTRL4           0x313U
#define BQ79718_ADDR_CB_CELL18_CTRL      0x320U   /* first byte of CB-timer block */
//#define BQ79718_ADDR_CB_CELL17_CTRL      0x321U
//#define BQ79718_ADDR_CB_CELL16_CTRL      0x322U
//#define BQ79718_ADDR_CB_CELL15_CTRL      0x323U
//#define BQ79718_ADDR_CB_CELL14_CTRL      0x324U
//#define BQ79718_ADDR_CB_CELL13_CTRL      0x325U
//#define BQ79718_ADDR_CB_CELL12_CTRL      0x326U
//#define BQ79718_ADDR_CB_CELL11_CTRL      0x327U
//#define BQ79718_ADDR_CB_CELL10_CTRL      0x328U
//#define BQ79718_ADDR_CB_CELL9_CTRL       0x329U
//#define BQ79718_ADDR_CB_CELL8_CTRL       0x32AU
//#define BQ79718_ADDR_CB_CELL7_CTRL       0x32BU
//#define BQ79718_ADDR_CB_CELL6_CTRL       0x32CU
//#define BQ79718_ADDR_CB_CELL5_CTRL       0x32DU
//#define BQ79718_ADDR_CB_CELL4_CTRL       0x32EU
//#define BQ79718_ADDR_CB_CELL3_CTRL       0x32FU
//#define BQ79718_ADDR_CB_CELL2_CTRL       0x330U
//#define BQ79718_ADDR_CB_CELL1_CTRL       0x331U   /* last  byte of CB-timer block */
#define BQ79718_ADDR_VCBDONE_THRESH      0x332U
#define BQ79718_ADDR_OTCB_THRESH         0x333U
#define BQ79718_ADDR_OVUV_CTRL1          0x334U
#define BQ79718_ADDR_OVUV_CTRL2          0x335U
#define BQ79718_ADDR_OTUT_CTRL1          0x336U
#define BQ79718_ADDR_OTUT_CTRL2          0x337U
#define BQ79718_ADDR_BAL_CTRL1           0x338U
#define BQ79718_ADDR_BAL_CTRL2           0x339U
#define BQ79718_ADDR_BAL_CTRL3           0x33AU
#define BQ79718_ADDR_FAULT_RST1          0x340U
#define BQ79718_ADDR_FAULT_RST2          0x341U
//#define BQ79718_ADDR_I2C_WR_DATA         0x370U
//#define BQ79718_ADDR_I2C_CTRL            0x371U
//#define BQ79718_ADDR_SPI_CONF            0x380U
//#define BQ79718_ADDR_SPI_TX3             0x381U
//#define BQ79718_ADDR_SPI_TX2             0x382U
//#define BQ79718_ADDR_SPI_TX1             0x383U
//#define BQ79718_ADDR_SPI_EXE             0x384U
//#define BQ79718_ADDR_OTP_PROG_UNLOCK2    0x4FFU
#define BQ79718_ADDR_OTP_ECC_TEST        0x350U
/* ---- Status Registers (0x500-0x6FF) ---- */
#define BQ79718_ADDR_PARTID              0x500U
#define BQ79718_ADDR_TAPEOUT_REV         0x501U
#define BQ79718_ADDR_DIE_ID1             0x502U
#define BQ79718_ADDR_DIE_ID2             0x503U
#define BQ79718_ADDR_DIE_ID3             0x504U
#define BQ79718_ADDR_DIE_ID4             0x505U
#define BQ79718_ADDR_DIE_ID5             0x506U
#define BQ79718_ADDR_DIE_ID6             0x507U
#define BQ79718_ADDR_DIE_ID7             0x508U
#define BQ79718_ADDR_DIE_ID8             0x509U
//#define BQ79718_ADDR_CUST_CRC_RSLT_HI    0x50CU
//#define BQ79718_ADDR_CUST_CRC_RSLT_LO    0x50DU
//#define BQ79718_ADDR_OTP_STAT            0x510U
//#define BQ79718_ADDR_GPIO_STAT1          0x520U
//#define BQ79718_ADDR_GPIO_STAT2          0x521U
//#define BQ79718_ADDR_BAL_STAT            0x522U
//#define BQ79718_ADDR_BAL_SW_STAT1        0x525U
//#define BQ79718_ADDR_BAL_SW_STAT2        0x526U
//#define BQ79718_ADDR_BAL_SW_STAT3        0x527U
//#define BQ79718_ADDR_BAL_DONE1           0x529U
//#define BQ79718_ADDR_BAL_DONE2           0x52AU
//#define BQ79718_ADDR_BAL_DONE3           0x52BU
//#define BQ79718_ADDR_BAL_TIME            0x52CU
//#define BQ79718_ADDR_ADC_DATA_RDY        0x52FU
#define BQ79718_ADDR_DEV_STAT1           0x530U
#define BQ79718_ADDR_DEV_STAT2           0x531U
#define BQ79718_ADDR_FAULT_SUMMARY       0x532U
//#define BQ79718_ADDR_FAULT_PWR1          0x535U
//#define BQ79718_ADDR_FAULT_PWR2          0x536U
//#define BQ79718_ADDR_FAULT_COMM          0x538U
//#define BQ79718_ADDR_FAULT_OTP           0x53BU
//#define BQ79718_ADDR_FAULT_SYS           0x53DU
//#define BQ79718_ADDR_FAULT_OV1           0x540U
//#define BQ79718_ADDR_FAULT_OV2           0x541U
//#define BQ79718_ADDR_FAULT_OV3           0x542U
//#define BQ79718_ADDR_FAULT_UV1           0x543U
//#define BQ79718_ADDR_FAULT_UV2           0x544U
//#define BQ79718_ADDR_FAULT_UV3           0x545U
//#define BQ79718_ADDR_FAULT_OT1           0x547U
//#define BQ79718_ADDR_FAULT_OT2           0x548U
//#define BQ79718_ADDR_FAULT_UT1           0x54AU
//#define BQ79718_ADDR_FAULT_UT2           0x54BU
//#define BQ79718_ADDR_FAULT_OC            0x55BU
#define BQ79718_ADDR_VCELL18_HI          0x574U   /* first byte of cell-voltage block */
//#define BQ79718_ADDR_VCELL18_LO          0x575U
//#define BQ79718_ADDR_VCELL17_HI          0x576U
//#define BQ79718_ADDR_VCELL17_LO          0x577U
//#define BQ79718_ADDR_VCELL16_HI          0x578U
//#define BQ79718_ADDR_VCELL16_LO          0x579U
//#define BQ79718_ADDR_VCELL15_HI          0x57AU
//#define BQ79718_ADDR_VCELL15_LO          0x57BU
//#define BQ79718_ADDR_VCELL14_HI          0x57CU
//#define BQ79718_ADDR_VCELL14_LO          0x57DU
//#define BQ79718_ADDR_VCELL13_HI          0x57EU
//#define BQ79718_ADDR_VCELL13_LO          0x57FU
//#define BQ79718_ADDR_VCELL12_HI          0x580U
//#define BQ79718_ADDR_VCELL12_LO          0x581U
//#define BQ79718_ADDR_VCELL11_HI          0x582U
//#define BQ79718_ADDR_VCELL11_LO          0x583U
//#define BQ79718_ADDR_VCELL10_HI          0x584U
//#define BQ79718_ADDR_VCELL10_LO          0x585U
//#define BQ79718_ADDR_VCELL9_HI           0x586U
//#define BQ79718_ADDR_VCELL9_LO           0x587U
//#define BQ79718_ADDR_VCELL8_HI           0x588U
//#define BQ79718_ADDR_VCELL8_LO           0x589U
//#define BQ79718_ADDR_VCELL7_HI           0x58AU
//#define BQ79718_ADDR_VCELL7_LO           0x58BU
//#define BQ79718_ADDR_VCELL6_HI           0x58CU
//#define BQ79718_ADDR_VCELL6_LO           0x58DU
//#define BQ79718_ADDR_VCELL5_HI           0x58EU
//#define BQ79718_ADDR_VCELL5_LO           0x58FU
//#define BQ79718_ADDR_VCELL4_HI           0x590U
//#define BQ79718_ADDR_VCELL4_LO           0x591U
//#define BQ79718_ADDR_VCELL3_HI           0x592U
//#define BQ79718_ADDR_VCELL3_LO           0x593U
//#define BQ79718_ADDR_VCELL2_HI           0x594U
//#define BQ79718_ADDR_VCELL2_LO           0x595U
//#define BQ79718_ADDR_VCELL1_HI           0x596U
#define BQ79718_ADDR_VCELL1_LO           0x597U

#define BQ79718_ADDR_VCELL_ACT_SUM_HI    0x598U
#define BQ79718_ADDR_VCELL_ACT_SUM_LO    0x599U
#define BQ79718_ADDR_BAT_HI              0x59AU
#define BQ79718_ADDR_BAT_LO              0x59BU

#define BQ79718_ADDR_GPIO1_HI            0x5A8U   /* first byte of GPIO-ADC block */
//#define BQ79718_ADDR_GPIO1_LO            0x5A9U
//#define BQ79718_ADDR_GPIO2_HI            0x5AAU
//#define BQ79718_ADDR_GPIO2_LO            0x5ABU
//#define BQ79718_ADDR_GPIO3_HI            0x5ACU
//#define BQ79718_ADDR_GPIO3_LO            0x5ADU
//#define BQ79718_ADDR_GPIO4_HI            0x5AEU
//#define BQ79718_ADDR_GPIO4_LO            0x5AFU
//#define BQ79718_ADDR_GPIO5_HI            0x5B0U
//#define BQ79718_ADDR_GPIO5_LO            0x5B1U
//#define BQ79718_ADDR_GPIO6_HI            0x5B2U
//#define BQ79718_ADDR_GPIO6_LO            0x5B3U
//#define BQ79718_ADDR_GPIO7_HI            0x5B4U
//#define BQ79718_ADDR_GPIO7_LO            0x5B5U
//#define BQ79718_ADDR_GPIO8_HI            0x5B6U
//#define BQ79718_ADDR_GPIO8_LO            0x5B7U
//#define BQ79718_ADDR_GPIO9_HI            0x5B8U
//#define BQ79718_ADDR_GPIO9_LO            0x5B9U
//#define BQ79718_ADDR_GPIO10_HI           0x5BAU
//#define BQ79718_ADDR_GPIO10_LO           0x5BBU
//#define BQ79718_ADDR_GPIO11_HI           0x5BCU
#define BQ79718_ADDR_GPIO11_LO           0x5BDU

#define BQ79718_ADDR_DIETEMP1_HI         0x5F0U
#define BQ79718_ADDR_DIETEMP1_LO         0x5F1U
#define BQ79718_ADDR_DIETEMP2_HI         0x5F2U
#define BQ79718_ADDR_DIETEMP2_LO         0x5F3U
//#define BQ79718_ADDR_REF_CAP_HI          0x5F4U
//#define BQ79718_ADDR_REF_CAP_LO          0x5F5U
//#define BQ79718_ADDR_I2C_RD_DATA         0x610U
//#define BQ79718_ADDR_SPI_RX3             0x620U
//#define BQ79718_ADDR_SPI_RX2             0x621U
//#define BQ79718_ADDR_SPI_RX1             0x622U
#ifdef __cplusplus
}
#endif

#endif /* BQ79718_ADDR_H */
