/*******************************************************************************
Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensor.
******************************************************************************
* @file:    adBms6830Data.h
* @brief:   Generic bms data structure header file
* @version: $Revision$
* @date:    $Date$
* Developed by: ADIBMS Software team, Bangalore, India
*****************************************************************************/

/**
 * @file adBms6830Data.h
 * @brief Data types for the ADBMS6830 driver: the per-chip @ref cell_asic
 *        struct, register-field structs, sizes, and option enums.
 *
 * The whole driver revolves around one array, `cell_asic IC[TOTAL_IC]`
 * (declared in adBms_Application.c), with one element per chip in the daisy
 * chain. Each element holds:
 * - **what you want to send** (`tx_cfga`, `tx_cfgb`, `PwmA`, ...),
 * - **what the chip sent back** (`rx_cfga`, `cell`, `aux`, `stata`, ...),
 * - **raw byte buffers** (`configa.tx_data`, `configa.rx_data`, ...), and
 * - **error-check results** (`cccrc`).
 *
 * @par Bit fields
 * Many structs use C bit fields, e.g. `uint8_t cth : 3;` means "this field is
 * only 3 bits wide" (values 0–7). The compiler packs them; you read and write
 * them like normal struct members.
 *
 * @ingroup BMS_DRIVER
 */

/*! @addtogroup BMS DRIVER
 *  @{
 *
 */

/*! @addtogroup BMS_DATA BMS DATA
 *  @{
 *
 */

#ifndef __ADBMSDATA_H
#define __ADBMSDATA_H

#include "common.h"

/** @name Sizes
 *  @{ */
#define CELL 16          /**< Cell inputs per ADBMS6830 (max 16 cells per chip). */
#define AUX 12           /**< Auxiliary (GPIO) result slots per chip. */
#define RAUX 10          /**< Redundant auxiliary result slots per chip. */
#define PWMA 12          /**< Balancing PWM channels in PWM register A. */
#define PWMB 4           /**< Balancing PWM channels in PWM register B. */
#define COMM 3           /**< Bytes in the COMM (GPIO I2C/SPI) register. */
#define RSID 6           /**< Bytes in the 48-bit serial ID. */
#define TX_DATA 6        /**< Data bytes per chip in a register write (PEC added separately). */
#define RX_DATA 8        /**< Bytes per chip in a register read: 6 data + 2 counter/PEC. */
#define RDCVALL_SIZE 34  /**< Bytes per chip for RDCVALL: 32 data (16 cells x 2) + 2 PEC. */
#define RDSALL_SIZE 34   /**< Bytes per chip for RDSALL. */
#define RDACALL_SIZE 34  /**< Bytes per chip for RDACALL. */
#define RDFCALL_SIZE 34  /**< Bytes per chip for RDFCALL. */
#define RDCSALL_SIZE 66  /**< Bytes per chip for RDCSALL (C + S results). */
#define RDASALL_SIZE 70  /**< Bytes per chip for RDASALL (aux + redundant aux + status). */
#define RDACSALL_SIZE 66 /**< Bytes per chip for RDACSALL. */
/** @} */

/** @brief Configuration Register A fields. */
typedef struct {
    uint8_t refon : 1;   /**< 1 = keep the voltage reference powered up between conversions. */
    uint8_t cth : 3;     /**< C-vs-S comparison threshold (@ref CTH). */
    uint8_t flag_d : 8;  /**< Diagnostic force flags, one bit per @ref FLAG_D. */
    uint8_t soakon : 1;  /**< Enable open-wire soak time. */
    uint8_t owrng : 1;   /**< Soak time range (@ref OWRNG). */
    uint8_t owa : 3;     /**< Soak time step (@ref OWA). */
    uint16_t gpo : 10;   /**< GPIO output levels, bit n = GPIO(n+1). */
    uint8_t snap : 1;    /**< Snapshot status. */
    uint8_t mute_st : 1; /**< Discharge mute status. */
    uint8_t comm_bk : 1; /**< Communication break (@ref COMM_BK). */
    uint8_t fc : 3;      /**< IIR filter setting (@ref IIR_FPA). */
} cfa_;

/** @brief Configuration Register B fields. */
typedef struct {
    /** Under-voltage threshold (12-bit code). Set with SetUnderVoltageThreshold(). */
    uint16_t vuv : 16;
    /** Over-voltage threshold (12-bit code). Set with SetOverVoltageThreshold(). */
    uint16_t vov : 16;
    uint8_t dtmen : 1; /**< Enable discharge timer monitor. */
    uint8_t dtrng : 1; /**< Discharge timer range (@ref DTRNG). */
    uint8_t dcto : 6;  /**< Discharge timeout (@ref DCTO). */
    uint16_t dcc : 16; /**< Balancing switch per cell: bit n = cell n+1. 1 = discharging. */
} cfb_;

/**
 * @brief Cell voltage results (C-ADC).
 *
 * Raw signed codes. Convert to volts with `c_codes[k] * 0.000150 + 1.5`
 * (150 uV per LSB, 1.5 V offset). Example: code 13500 -> 3.525 V.
 */
typedef struct {
    int16_t c_codes[CELL]; /**< Raw code for cell k (index 0 = cell 1). */
} cv_;

typedef struct {
    int16_t ac_codes[CELL]; /* Average Cell Voltage Codes */
} acv_;

/* S Voltage Data structure */
typedef struct {
    int16_t sc_codes[CELL]; /* S Voltage Codes */
} scv_;

/* Filtered Cell Voltage Data structure */
typedef struct {
    int16_t fc_codes[CELL]; /* filtered Cell Voltage Codes */
} fcv_;

/* Aux Voltage Data Structure*/
typedef struct {
    int16_t a_codes[AUX]; /* Aux Voltage Codes */
} ax_;

/* Redundant Aux Voltage Data Structure*/
typedef struct {
    int16_t ra_codes[RAUX]; /* Aux Voltage Codes */
} rax_;

/* Status A register Data structure*/
typedef struct {
    uint16_t vref2;
    uint16_t itmp;
    uint16_t vref3;
} sta_;

/* Status B register Data structure*/
typedef struct {
    uint16_t vd;
    uint16_t va;
    uint16_t vr4k;
} stb_;

/* Status C register Data structure*/
typedef struct {
    uint16_t cs_flt;
    uint8_t va_ov : 1;
    uint8_t va_uv : 1;
    uint8_t vd_ov : 1;
    uint8_t vd_uv : 1;
    uint8_t otp1_ed : 1;
    uint8_t otp1_med : 1;
    uint8_t otp2_ed : 1;
    uint8_t otp2_med : 1;
    uint8_t vde : 1;
    uint8_t vdel : 1;
    uint8_t comp : 1;
    uint8_t spiflt : 1;
    uint8_t sleep : 1;
    uint8_t thsd : 1;
    uint8_t tmodchk : 1;
    uint8_t oscchk : 1;
} stc_;

/* ClrFlag register Data structure*/
typedef struct {
    uint16_t cl_csflt;
    uint8_t cl_smed : 1;
    uint8_t cl_sed : 1;
    uint8_t cl_cmed : 1;
    uint8_t cl_ced : 1;
    uint8_t cl_vduv : 1;
    uint8_t cl_vdov : 1;
    uint8_t cl_vauv : 1;
    uint8_t cl_vaov : 1;
    uint8_t cl_oscchk : 1;
    uint8_t cl_tmode : 1;
    uint8_t cl_thsd : 1;
    uint8_t cl_sleep : 1;
    uint8_t cl_spiflt : 1;
    uint8_t cl_vdel : 1;
    uint8_t cl_vde : 1;
} clrflag_;

/* Status D register Data structure*/
typedef struct {
    uint8_t c_ov[CELL];
    uint8_t c_uv[CELL];
    uint8_t ct : 6;
    uint8_t cts : 2;
    uint8_t oc_cntr;
} std_;

/* Status E register Data structure*/
typedef struct {
    uint16_t gpi : 10;
    uint8_t rev : 4;
} ste_;

/* Pwm register Data structure*/
typedef struct {
    uint8_t pwma[PWMA];
} pwma_;

/*PWMB Register Structure */
typedef struct {
    uint8_t pwmb[PWMB];
} pwmb_;

/* COMM register Data structure*/
typedef struct {
    uint8_t fcomm[COMM];
    uint8_t icomm[COMM];
    uint8_t data[COMM];
} com_;

/*SID Register Structure */
typedef struct {
    uint8_t sid[RSID];
} sid_;

/* Transmit byte and recived byte data structure */
typedef struct {
    uint8_t tx_data[TX_DATA];
    uint8_t rx_data[RX_DATA];
} ic_register_;

/**
 * @brief Command counter and PEC-error flags for one chip.
 *
 * After each adBmsReadData() call, the flag for that kind of data is 0 if the
 * data passed its CRC-10 check, or 1 if it was corrupted and must not be
 * trusted. The application treats repeated errors as a lost connection.
 */
typedef struct {
    uint8_t cmd_cntr; /**< Chip's 6-bit command counter from the last read. */
    uint8_t cfgr_pec;
    uint8_t cell_pec;
    uint8_t acell_pec;
    uint8_t scell_pec;
    uint8_t fcell_pec;
    uint8_t aux_pec;
    uint8_t raux_pec;
    uint8_t stat_pec;
    uint8_t comm_pec;
    uint8_t pwm_pec;
    uint8_t sid_pec;
} cmdcnt_pec_;

/* Diagnostic test result data structure */
typedef struct {
    uint8_t osc_mismatch;
    uint8_t supply_error;
    uint8_t supply_ovuv;
    uint8_t thsd;
    uint8_t fuse_ed;
    uint8_t fuse_med;
    uint8_t tmodchk;
    uint8_t cell_ow[CELL];
    uint8_t cellred_ow[CELL];
    uint8_t aux_ow[(AUX - 2)];
} diag_test_;

/* Aux open wire data structure */
typedef struct {
    int cell_ow_even[CELL];
    int cell_ow_odd[CELL];
} cell_ow_;

/* Aux open wire data structure */
typedef struct {
    int aux_pup_up[(AUX - 2)];
    int aux_pup_down[(AUX - 2)];
} aux_ow_;

/**
 * @brief Everything the driver knows about one ADBMS6830 chip.
 *
 * Make an array of these, one per chip in the daisy chain, and pass it to
 * the driver functions. Element 0 is the chip closest to the MCU.
 */
typedef struct {
    cfa_ tx_cfga;           /**< Config A to WRITE (you set this). */
    cfa_ rx_cfga;           /**< Config A as READ back from the chip. */
    cfb_ tx_cfgb;           /**< Config B to WRITE (thresholds, balancing bits). */
    cfb_ rx_cfgb;           /**< Config B as READ back. */
    clrflag_ clflag;        /**< Fault flags to clear with CLRFLAG. */
    cv_ cell;               /**< Cell voltages (main ADC). */
    acv_ acell;             /**< Averaged cell voltages. */
    scv_ scell;             /**< Redundant (S-ADC) cell voltages. */
    fcv_ fcell;             /**< Filtered cell voltages. */
    ax_ aux;                /**< Aux/GPIO voltages (temperature sensors). */
    rax_ raux;              /**< Redundant aux voltages. */
    sta_ stata;             /**< Status A: references, die temperature. */
    stb_ statb;             /**< Status B: supply voltages. */
    stc_ statc;             /**< Status C: fault bits. */
    std_ statd;             /**< Status D: per-cell OV/UV flags. */
    ste_ state;             /**< Status E: GPIO inputs, revision. */
    com_ comm;              /**< COMM register contents. */
    pwma_ PwmA;             /**< Balancing PWM duty, cells 1-12 (@ref PWM_DUTY). */
    pwmb_ PwmB;             /**< Balancing PWM duty, cells 13-16. */
    sid_ sid;               /**< 48-bit serial ID, sid[0] = least significant byte. */
    ic_register_ configa;   /**< Raw bytes for Config A (tx_data sent, rx_data received). */
    ic_register_ configb;   /**< Raw bytes for Config B. */
    ic_register_ clrflag;   /**< Raw bytes for CLRFLAG. */
    ic_register_ stat;      /**< Raw bytes for the last status register read. */
    ic_register_ com;       /**< Raw bytes for COMM. */
    ic_register_ pwma;      /**< Raw bytes for PWM A. Rebuilt from PwmA on every write. */
    ic_register_ pwmb;      /**< Raw bytes for PWM B. Rebuilt from PwmB on every write. */
    ic_register_ rsid;      /**< Raw bytes for the serial ID. */
    cmdcnt_pec_ cccrc;      /**< PEC error flags and command counter. */
    aux_ow_ gpio;           /**< Aux open-wire test scratch data. */
    cell_ow_ owcell;        /**< Cell open-wire test scratch data. */
    diag_test_ diag_result; /**< Results of the built-in self tests. */
} cell_asic;

/*!
 *  \enum Loop Measurement ENABLED or DISABLED.
 */
typedef enum { DISABLED = 0X0, ENABLED = 0X1 } LOOP_MEASURMENT;

/**************************************** CMDEnums
 * *************************************************/
/*!
 *  \enum GPIO CHANNEL
 * CH: GPIO Channels.
 */
/* Channel selection */
typedef enum {
    AUX_ALL = 0,
    GPIO1,
    GPIO2,
    GPIO3,
    GPIO4,
    GPIO5,
    GPIO6,
    GPIO7,
    GPIO8,
    GPIO9,
    GPIO10,
    VREF2,
    LD03V,
    LD05V,
    TEMP,
    V_POSTIVE_2_NAGATIVE,
    V_NAGATIVE,
    VR4K,
    VREF3
} CH;

/*!
 *  \enum RD
 * RD: Read Device.
 */
typedef enum { RD_OFF = 0X0, RD_ON = 0X1 } RD;

/*!
 *  \enum CONT
 * CONT: Continuous or single measurement.
 */
/* Continuous or single measurement */
typedef enum { SINGLE = 0X0, CONTINUOUS = 0X1 } CONT;

/*!
 *  \enum OW_C_S
 * OW_C_S: Open wire c/s.
 */
/* Open wire c/s adcs */
typedef enum { OW_OFF_ALL_CH = 0X0, OW_ON_EVEN_CH, OW_ON_ODD_CH, OW_ON_ALL_CH } OW_C_S;

/*!
 *  \enum OW_AUX
 * OW_AUX: Open wire Aux.
 */
/* Open wire AUX */
typedef enum { AUX_OW_OFF = 0X0, AUX_OW_ON = 0X1 } OW_AUX;

/*!
 *  \enum PUP
 * PUP: Pull Down current during aux conversion.
 */
/* Pull Down current during aux conversion (if OW = 1) */
typedef enum { PUP_DOWN = 0X0, PUP_UP = 0X1 } PUP;

/*!
 *  \enum DCP
 * DCP: Discharge permitted.
 */
/* Discharge permitted */
typedef enum { DCP_OFF = 0X0, DCP_ON = 0X1 } DCP;

/*!
 *  \enum RSTF
 * RSTF: Reset Filter.
 */
/* Reset filter */
typedef enum { RSTF_OFF = 0x0, RSTF_ON = 0x1 } RSTF;

/*!
 *  \enum ERR
 * ERR: Inject error is spi read out.
 */
/* Inject error is spi read out */
typedef enum { WITHOUT_ERR = 0x0, WITH_ERR = 0x1 } ERR;

/**************************************** Mem bits
 * *************************************************/
/* Configuration Register A */

/*!
 *  \enum REFON
 * REFON: Refernece remains power up/down.
 */
/* Refernece remains power up/down */
typedef enum { PWR_DOWN = 0x0, PWR_UP = 0x1 } REFON;

/*!
 *  \enum CTH
 * CTH: Comparison voltages threshold C vs S.
 */
/* Comparison voltages threshold C vs S*/
typedef enum {
    CVT_5_1mV = 0, /* 5.1mV                */
    CVT_8_1mV,     /* 8.1mV (Default)      */
    CVT_10_05mV,   /* 10.05mV              */
    CVT_15mV,      /* 15mV                 */
    CVT_22_5mV,    /* 22.5mV               */
    CVT_45mV,      /* 45mV                 */
    CVT_75mV,      /* 75mV                 */
    CVT_135mV,     /* 135mV                */
} CTH;

/*!
 *  \enum FLAG_D
 * FLAG_D: Fault flags.
 */
/* Fault flags */
typedef enum {
    FLAG_D0 = 0, /* Force oscillator counter fast */
    FLAG_D1,     /* Force oscillator counter slow */
    FLAG_D2,     /* Force Supply Error detection  */
    FLAG_D3,     /* FLAG_D[3]: 1--> Select Supply OV and delta detection, 0 --> Selects UV */
    FLAG_D4,     /* Set THSD */
    FLAG_D5,     /* Force Fuse ED */
    FLAG_D6,     /* Force Fuse MED */
    FLAG_D7,     /* Force TMODCHK  */
} FLAG_D;

typedef enum { FLAG_CLR = 0x0, FLAG_SET = 0x1 } CFGA_FLAG;

/*!
 *  \enum CL FLAG
 * FLAG: Fault Clear.
 */
typedef enum { CL_FLAG_CLR = 0x0, CL_FLAG_SET = 0x1 } FLAG;

/*!
 *  \enum SOAKON
 * SOAKON: Enables or disable soak time for all commands.
 */
/* Enables or disable soak time for all commands */
typedef enum { SOAKON_CLR = 0x0, SOAKON_SET = 0x1 } SOAKON;

/* Open wire sokon time owa */
typedef enum { OWA0 = 0x0, OWA1, OWA2, OWA3, OWA4, OWA5, OWA6, OWA7 } OWA;

/*!
 *  \enum OWRNG
 * OWRNG: Set soak time range Long/Short.
 */
/* Set soak time range Long/Short */
typedef enum { SHORT = 0x0, LONG = 0x1 } OWRNG;

/*!
 *  \enum OW_TIME
 * OW_TIME:Open Wire Soak times
 *          For Aux commands. If OWRNG=0, Soak time = 2^(6 +OWA[2:0]) Clocks (32 us 4.1 ms)
 *          For Aux commands. If OWRNG=1, Soak time = 2^(13+OWA[2:0]) Clocks (41 ms 524 ms)
 */
typedef enum { TIME_32US_TO_4_1MS = 0x0, TIME_41MS_TO_524MS = 0x1 } OW_TIME;

/*!
 *  \enum GPO
 * GPO: GPO Pins.
 */
/* GPO Pins */
typedef enum {
    GPO1 = 0,
    GPO2,
    GPO3,
    GPO4,
    GPO5,
    GPO6,
    GPO7,
    GPO8,
    GPO9,
    GPO10,
} GPO;

/*!
 *  \enum GPIO
 * GPIO: GPIO Pin Control.
 */
/* GPO Pin Control */
typedef enum { GPO_CLR = 0x0, GPO_SET = 0x1 } CFGA_GPO;

/*!
 *  \enum IIR_FPA
 * IIR_FPA: IIR Filter Parameter.
 */
/* IIR Filter Parameter */
typedef enum {
    IIR_FPA_OFF = 0, /* Filter Disabled          */
    IIR_FPA2,        /* 110   Hz -3dB Frequency  */
    IIR_FPA4,        /* 45    Hz -3dB Frequency  */
    IIR_FPA8,        /* 21    Hz -3dB Frequency  */
    IIR_FPA16,       /* 10    Hz -3dB Frequency  */
    IIR_FPA32,       /* 5     Hz -3dB Frequency  */
    IIR_FPA128,      /* 1.25  Hz -3dB Frequency  */
    IIR_FPA256,      /* 0.625 Hz -3dB Frequency  */
} IIR_FPA;

/*!
 *  \enum COMM_BK
 * COMM_BK: Communication Break.
 */
/* Communication Break */
typedef enum { COMM_BK_OFF = 0x0, COMM_BK_ON = 0x1 } COMM_BK;

/*!
 *  \enum SNAPSHOT
 * SNAPSHOT: Snapshot.
 */
/* Snapshot */
typedef enum { SNAP_OFF = 0x0, SNAP_ON = 0x1 } SNAPSHOT;

/* Configuration Register B */

/*!
 *  \enum DTMEN
 * DTMEN: Enable Dis-charge Timer Monitor.
 */
/* Enable Dis-charge Timer Monitor */
typedef enum { DTMEN_OFF = 0x0, DTMEN_ON = 0x1 } DTMEN;

/*!
 *  \enum DTRNG
 * DTRNG: Discharge Timer Range Setting.
 */
/* Discharge Timer Range Setting */
typedef enum { RANG_0_TO_63_MIN = 0x0, RANG_0_TO_16_8_HR = 0x1 } DTRNG;

/*!
 *  \enum DCTO
 * DCTO: DCTO timeout values.
 */
typedef enum {
    DCTO_TIMEOUT = 0,
    TIME_1MIN_OR_0_26HR,
    TIME_2MIN_OR_0_53HR,
    TIME_3MIN_OR_0_8HR,
    TIME_4MIN_OR_1_06HR,
    TIME_5MIN_OR_1_33HR,
    TIME_6MIN_OR_1_6HR,
    /* If required more time out value add here */
} DCTO;

/*!
 *  \enum PWM
 * PWM: PWM Duty cycle.
 */
typedef enum {
    PWM_0_0_PCT = 0, /* 0.0%  (default) */
    PWM_6_6_PCT,     /* 6.6%            */
    PWM_13_2_PCT,    /* 13.2%           */
    PWM_19_8_PCT,    /* 19.8%           */
    PWM_26_4_PCT,    /* 26.4%           */
    PWM_33_0_PCT,    /* 33.0%           */
    PWM_39_6_PCT,    /* 39.6%           */
    PWM_46_2_PCT,    /* 46.2%           */
    PWM_52_8_PCT,    /* 52.8%           */
    PWM_59_4_PCT,    /* 59.4%           */
    PWM_66_0_PCT,    /* 66.0%           */
    PWM_72_6_PCT,    /* 72.6%           */
    PWM_79_2_PCT,    /* 79.2%           */
    PWM_85_8_PCT,    /* 85.8%           */
    PWM_92_4_PCT,    /* 92.4%           */
    PWM_100_0_PCT,   /* ~100.0%         */
} PWM_DUTY;

/*!
 *  \enum DCC
 * DCC: DCC bits.
 */
/* DCC bits */
typedef enum {
    DCC1 = 0,
    DCC2,
    DCC3,
    DCC4,
    DCC5,
    DCC6,
    DCC7,
    DCC8,
    DCC9,
    DCC10,
    DCC11,
    DCC12,
    DCC13,
    DCC14,
    DCC15,
    DCC16,
} DCC;

/*!
 *  \enum DCC_BIT
 * DCC_BIT: Discharge cell set and claer.
 */
/* Discharge cell set and claer  */
typedef enum { DCC_BIT_CLR = 0x0, DCC_BIT_SET = 0x1 } DCC_BIT;

/**
 * @brief Register group selector.
 *
 * Results are split across several 6-byte register groups (A, B, C, ...).
 * @c ALL_GRP is used with the "read all" commands (RDCVALL etc.); @c NONE is
 * for registers that have no groups (e.g. the serial ID).
 */
typedef enum { ALL_GRP = 0x0, A, B, C, D, E, F, NONE } GRP;
/**
 * @brief What kind of register is being read or written. Tells
 *        adBmsReadData() / adBmsWriteData() which parse/create function to use.
 */
typedef enum {
    Cell = 0x0,
    Aux,
    RAux,
    Status,
    Pwm,
    AvgCell,
    S_volt,
    F_volt,
    Config,
    Comm,
    Sid,
    Clrflag,
    Rdcvall,
    Rdacall,
    Rdsall,
    Rdcsall,
    Rdacsall,
    Rdfcall,
    Rdasall
} TYPE;
typedef enum { PASS, FAIL } RESULT;
typedef enum { OSC_MISMATCH = 0x0, SUPPLY_ERROR, THSD, FUSE_ED, FUSE_MED, TMODCHK } DIAGNOSTIC_TYPE;
#endif /* __BMS_DATA_H */

/** @}*/
/** @}*/
