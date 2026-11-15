/*******************************************************************************
Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensor.
******************************************************************************
* @file:    adBms6830ParseCreate.h
* @brief:   Data parse create helper function header file
* @version: $Revision$
* @date:    $Date$
* Developed by: ADIBMS Software team, Bangalore, India
*****************************************************************************/

/**
 * @file adBms6830ParseCreate.h
 * @brief Translate between raw register bytes and the fields of @ref cell_asic.
 *
 * - **Parse** functions (`adBms6830Parse*`) take bytes *received* from the
 *   chips and unpack them into named fields (e.g. cell voltage codes, fault
 *   bits).
 * - **Create** functions (`adBms6830Create*`) take the `tx_*` fields you set
 *   and pack them into the 6 bytes per IC that get *sent*.
 * - **Set/Config helpers** turn human values (volts, a pin number) into the
 *   register values the chip expects.
 *
 * None of these functions touch hardware: they only read and write memory.
 * That makes all of them straightforward to unit test with hand-built byte
 * arrays.
 *
 * @par Common parameter conventions
 * - `tIC`: number of ICs in the daisy chain; every function loops over
 *   `ic[0]` .. `ic[tIC-1]`.
 * - `ic`: pointer to the first element of a `cell_asic[tIC]` array.
 * - `data` / `*_data` (Parse): raw bytes for all ICs, laid out back to back
 *   as `RX_DATA` (8) bytes per IC for single-group reads: 6 data bytes +
 *   2 PEC/counter bytes. IC 0 comes first.
 * - `grp`: which register group the bytes came from (@c A .. @c F, or
 *   @c ALL_GRP). It decides which array slots get filled.
 *
 * @par Byte order
 * Measurement results are 16-bit, **little-endian** (low byte first):
 * `code = data[0] | (data[1] << 8)`. Store them in `int16_t` because the
 * codes are signed.
 *
 * @ingroup BMS_DRIVER
 */

/** @addtogroup BMS_DRIVER
 *  @{
 */

/** @defgroup PARSE_CREATE Parse / create register data
 *  @brief Pure functions that pack and unpack register bytes.
 *  @{
 */
#ifndef __ADBMSPARSECREATE_H
#define __ADBMSPARSECREATE_H

#include "adBms6830Data.h"
#include "common.h"

/** @name Value helpers (human units → register values)
 *  @{
 */

/**
 * @brief Convert an over-voltage threshold in volts to the 12-bit VOV
 *        register value.
 *
 * Formula: `(uint16_t)((volt - 1.5) / (16 * 150e-6) + 4096) & 0xFFF`.
 * Each threshold step is 16 x 150 µV = 2.4 mV above 1.5 V. Adding 4096 and
 * masking to 12 bits turns a negative result (volt < 1.5 V) into its 12-bit
 * two's-complement form; for normal thresholds it has no effect.
 * Example: 4.25 V -> 1145, 2.5 V -> 416. The fraction is truncated, not
 * rounded.
 *
 * @param[in] volt Threshold in volts (e.g. 4.25f).
 * @return 12-bit value to store in `ic[n].tx_cfgb.vov`.
 */
uint16_t SetOverVoltageThreshold(float volt);

/**
 * @brief Convert an under-voltage threshold in volts to the 12-bit VUV
 *        register value. Same formula as SetOverVoltageThreshold().
 *
 * @param[in] voltage Threshold in volts (e.g. 2.5f).
 * @return 12-bit value to store in `ic[n].tx_cfgb.vuv`.
 */
uint16_t SetUnderVoltageThreshold(float voltage);

/**
 * @brief Return a bit mask with the bit for one fault-test flag set.
 *
 * @param[in] flag_d Which flag (@c FLAG_D0 .. @c FLAG_D7).
 * @param[in] flag   @c FLAG_SET returns `1 << flag_d`; @c FLAG_CLR returns 0.
 * @return Mask to OR into `tx_cfga.flag_d`.
 *
 * @note Returning 0 for @c FLAG_CLR does not clear anything by itself. To
 *       clear a bit, the caller must do `flag_d &= ~(1 << flag_d)`.
 */
uint8_t ConfigA_Flag(FLAG_D flag_d, CFGA_FLAG flag);

/**
 * @brief Return a bit mask for one general-purpose output pin.
 *
 * @param[in] gpo  Pin (@c GPO1 .. @c GPO10).
 * @param[in] stat @c GPO_SET returns `1 << gpo`; @c GPO_CLR returns 0.
 * @return Mask to OR into `tx_cfga.gpo`. Same clearing note as
 *         ConfigA_Flag().
 */
uint16_t ConfigA_Gpo(GPO gpo, CFGA_GPO stat);

/**
 * @brief Return a bit mask for one cell's discharge (balancing) switch.
 *
 * @param[in] dcc    Cell (@c DCC1 .. @c DCC16).
 * @param[in] dccbit @c DCC_BIT_SET returns `1 << dcc`; @c DCC_BIT_CLR
 *                   returns 0.
 * @return Mask to OR into `tx_cfgb.dcc`.
 */
uint16_t ConfigB_DccBit(DCC dcc, DCC_BIT dccbit);

/**
 * @brief Set the discharge-timer range and timeout in every IC's `tx_cfgb`.
 *
 * Only updates memory. Call adBmsWriteData() with `WRCFGB` afterwards to send
 * it.
 *
 * @param[in]     tIC           Number of ICs.
 * @param[in,out] ic            IC array; writes `tx_cfgb.dtrng` and
 *                              `tx_cfgb.dcto`.
 * @param[in]     timer_rang    @c RANG_0_TO_63_MIN or @c RANG_0_TO_16_8_HR.
 * @param[in]     timeout_value Timeout step (@ref DCTO).
 */
void SetConfigB_DischargeTimeOutValue(uint8_t tIC, cell_asic* ic, DTRNG timer_rang,
                                      DCTO timeout_value);

/**
 * @brief Set the same balancing PWM duty cycle for every cell of every IC.
 *
 * Only updates memory (`PwmA.pwma[]`, `PwmB.pwmb[]`). Send it with
 * adBmsWriteData(..., Pwm, A) and (..., Pwm, B).
 *
 * @param[in]     tIC        Number of ICs.
 * @param[in,out] ic         IC array.
 * @param[in]     duty_cycle Duty cycle step (@ref PWM_DUTY).
 */
void SetPwmDutyCycle(uint8_t tIC, cell_asic* ic, PWM_DUTY duty_cycle);
/** @} */

/** @name Parse functions (received bytes → struct fields)
 *  Each one copies the raw bytes for IC n into `ic[n].<reg>.rx_data[]`
 *  (where applicable) and then decodes the bit fields.
 *  @{
 */

/** @brief Decode Configuration Register A into `ic[n].rx_cfga`.
 *  @param[in] tIC Number of ICs.
 *  @param[in,out] ic IC array.
 *  @param[in] data `8 * tIC` received bytes. */
void adBms6830ParseConfiga(uint8_t tIC, cell_asic* ic, uint8_t* data);

/** @brief Decode Configuration Register B into `ic[n].rx_cfgb`
 *         (VUV, VOV, discharge timer, DCC bits).
 *  @param[in] tIC Number of ICs.
 *  @param[in,out] ic IC array.
 *  @param[in] data `8 * tIC` received bytes. */
void adBms6830ParseConfigb(uint8_t tIC, cell_asic* ic, uint8_t* data);

/** @brief Dispatch to ParseConfiga() (@p grp = @c A) or ParseConfigb()
 *         (@p grp = @c B). Other groups are ignored. */
void adBms6830ParseConfig(uint8_t tIC, cell_asic* ic, GRP grp, uint8_t* data);

/**
 * @brief Decode cell-voltage result bytes into `ic[n].cell.c_codes[]`.
 *
 * Each register group holds 3 cells (group F holds 1):
 * | grp | fills c_codes[] |
 * |-----|-----------------|
 * | A   | 0, 1, 2         |
 * | B   | 3, 4, 5         |
 * | C   | 6, 7, 8         |
 * | D   | 9, 10, 11       |
 * | E   | 12, 13, 14      |
 * | F   | 15              |
 * | ALL_GRP | 0 .. 15 (expects 34 bytes per IC) |
 *
 * Convert a code to volts with `code * 0.000150 + 1.5`.
 *
 * @param[in]     tIC     Number of ICs.
 * @param[in,out] ic      IC array.
 * @param[in]     grp     Register group the bytes came from.
 * @param[in]     cv_data Received bytes for all ICs.
 */
void adBms6830ParseCell(uint8_t tIC, cell_asic* ic, GRP grp, uint8_t* cv_data);

/** @brief Same layout as ParseCell(), but fills `ic[n].acell.ac_codes[]`
 *         (averaged cell voltages). */
void adBms6830ParseAverageCell(uint8_t tIC, cell_asic* ic, GRP grp, uint8_t* acv_data);

/** @brief Same layout as ParseCell(), but fills `ic[n].scell.sc_codes[]`
 *         (redundant S-ADC cell voltages). */
void adBms6830ParseSCell(uint8_t tIC, cell_asic* ic, GRP grp, uint8_t* scv_data);

/** @brief Same layout as ParseCell(), but fills `ic[n].fcell.fc_codes[]`
 *         (IIR-filtered cell voltages). */
void adBms6830ParseFCell(uint8_t tIC, cell_asic* ic, GRP grp, uint8_t* fcv_data);

/**
 * @brief Decode auxiliary (GPIO) results into `ic[n].aux.a_codes[]`.
 *
 * Groups A–C fill 3 codes each (indices 0–8); group D fills the rest.
 * The pack's temperature sensors are read from these codes in
 * adBms_Application.c.
 */
void adBms6830ParseAux(uint8_t tIC, cell_asic* ic, GRP grp, uint8_t* aux_data);

/** @brief Decode redundant auxiliary results into `ic[n].raux.ra_codes[]`. */
void adBms6830ParseRAux(uint8_t tIC, cell_asic* ic, GRP grp, uint8_t* raux_data);

/** @brief Status A → `ic[n].stata` (VREF2, internal die temperature ITMP,
 *         VREF3). */
void adBms6830ParseStatusA(uint8_t tIC, cell_asic* ic, uint8_t* data);

/** @brief Status B → `ic[n].statb` (digital and analog supply voltages). */
void adBms6830ParseStatusB(uint8_t tIC, cell_asic* ic, uint8_t* data);

/** @brief Status C → `ic[n].statc` (C-vs-S fault bits and diagnostic flags). */
void adBms6830ParseStatusC(uint8_t tIC, cell_asic* ic, uint8_t* data);

/**
 * @brief Status D → `ic[n].statd`: per-cell over/under-voltage flags.
 *
 * Bytes 0–3 hold 2 bits per cell (UV then OV), 4 cells per byte, so
 * `c_uv[k]` / `c_ov[k]` are 1 if cell k crossed the VUV / VOV threshold.
 */
void adBms6830ParseStatusD(uint8_t tIC, cell_asic* ic, uint8_t* data);

/** @brief Status E → `ic[n].state` (GPIO input states, silicon revision). */
void adBms6830ParseStatusE(uint8_t tIC, cell_asic* ic, uint8_t* data);

/** @brief Dispatch to the ParseStatusA..E() function matching @p grp. */
void adBms6830ParseStatus(uint8_t tIC, cell_asic* ic, GRP grp, uint8_t* data);

/** @brief Decode the COMM register (GPIO I2C/SPI passthrough) into
 *         `ic[n].comm`. */
void adBms6830ParseComm(uint8_t tIC, cell_asic* ic, uint8_t* data);

/** @brief Decode PWM register A into `ic[n].PwmA.pwma[]`. */
void adBms6830ParsePwma(uint8_t tIC, cell_asic* ic, uint8_t* data);

/** @brief Decode PWM register B into `ic[n].PwmB.pwmb[]`. */
void adBms6830ParsePwmb(uint8_t tIC, cell_asic* ic, uint8_t* data);

/** @brief Dispatch to ParsePwma() (@c A) or ParsePwmb() (@c B). */
void adBms6830ParsePwm(uint8_t tIC, cell_asic* ic, GRP grp, uint8_t* data);

/**
 * @brief Copy each IC's 6-byte serial ID into `ic[n].sid.sid[0..5]`.
 *
 * The application combines the 6 bytes into a 48-bit number and looks it up
 * with lookupID() to work out which physical board each IC is on.
 */
void adBms6830ParseSID(uint8_t tIC, cell_asic* ic, uint8_t* data);
/** @} */

/** @name Create functions (struct fields → bytes to send)
 *  Each one reads the `tx_*` fields of every IC and writes 6 packed bytes
 *  into the matching `ic[n].<reg>.tx_data[]`. adBmsWriteData() calls these
 *  for you.
 *  @{
 */

/** @brief Pack `tx_cfga` into `configa.tx_data[6]`. */
void adBms6830CreateConfiga(uint8_t tIC, cell_asic* ic);

/**
 * @brief Pack `tx_cfgb` into `configb.tx_data[6]`.
 *
 * Byte layout: [0] VUV[7:0], [1] VOV[3:0]<<4 | VUV[11:8], [2] VOV[11:4],
 * [3] DTMEN<<7 | DTRNG<<6 | DCTO, [4] DCC[7:0], [5] DCC[15:8].
 */
void adBms6830CreateConfigb(uint8_t tIC, cell_asic* ic);

/** @brief Pack the `clflag` bits into `clrflag.tx_data[6]` (for CLRFLAG). */
void adBms6830CreateClrflagData(uint8_t tIC, cell_asic* ic);

/** @brief Pack `comm` into `com.tx_data[6]` (for WRCOMM). */
void adBms6830CreateComm(uint8_t tIC, cell_asic* ic);

/** @brief Pack PWM A duty cycles into `pwma.tx_data[6]`. */
void adBms6830CreatePwma(uint8_t tIC, cell_asic* ic);

/** @brief Pack PWM B duty cycles into `pwmb.tx_data[6]`. */
void adBms6830CreatePwmb(uint8_t tIC, cell_asic* ic);
/** @} */

#endif
/** @}*/
/** @}*/
