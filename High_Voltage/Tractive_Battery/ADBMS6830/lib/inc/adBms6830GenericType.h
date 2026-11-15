/*******************************************************************************
Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensor.
******************************************************************************
* @file:    adBms6830GenericType.h
* @brief:   Generic Type function header file
* @version: $Revision$
* @date:    $Date$
* Developed by: ADIBMS Software team, Bangalore, India
*****************************************************************************/

/**
 * @file adBms6830GenericType.h
 * @brief Core ADBMS6830 protocol layer: PEC (CRC) math, SPI command framing,
 *        daisy-chain register read/write, and ADC start commands.
 *
 * This is the layer that "speaks" the ADBMS6830 SPI protocol. Everything
 * above it (adBms_Application.c) deals in @ref cell_asic structs; everything
 * below it (mcuWrapper.c) deals in raw bytes on the SPI bus.
 *
 * @par Frame format (what actually goes on the wire)
 * Every transaction starts with a 4-byte command frame:
 * @code
 *   [ CMD0 ][ CMD1 ][ PEC15_HI ][ PEC15_LO ]
 * @endcode
 * The PEC (Packet Error Code) is a CRC-15 computed over CMD0..CMD1 by
 * Pec15_Calc(). Register data that follows is sent/received in 8-byte
 * chunks per IC: 6 data bytes + a 2-byte CRC-10 (see pec10_calc()). On
 * reads, the top 6 bits of byte 6 hold the chip's command counter.
 *
 * @par Daisy chain
 * Several ADBMS6830s are chained on one isoSPI bus. On a write, the first
 * data chunk shifted out ends up in the IC *farthest* from the MCU, so
 * spiWriteData() sends IC data in reverse order. On a read, data comes back
 * IC 0 first, so the receive buffer is indexed as `ic * bytes_per_ic`.
 *
 * @ingroup BMS_DRIVER
 */

/** @addtogroup BMS_DRIVER
 *  @{
 */

/** @defgroup GENERIC_TYPE Protocol layer (PEC, SPI framing, ADC commands)
 *  @brief Functions that build, send and check ADBMS6830 SPI frames.
 *  @{
 */
#ifndef __adBmsGenericType_H
#define __adBmsGenericType_H

#include "adbms_main.h"

/**
 * @brief Compute the 15-bit command PEC (CRC-15) for a byte array.
 *
 * Used for the 2 command bytes at the start of every SPI transaction.
 * Table-driven: each input byte looks up a precomputed value in
 * `Crc15Table[256]`. The seed is 16 (0x0010), per the datasheet.
 *
 * @param[in] len   Number of bytes in @p data to include (normally 2).
 * @param[in] data  Bytes to protect. Not modified.
 * @return The PEC shifted left by 1 (bit 0 is always 0), ready to send
 *         as two bytes: `pec >> 8` then `pec & 0xFF`.
 *
 * @note Pure function: no hardware access. Easiest function in the driver
 *       to unit test.
 */
uint16_t Pec15_Calc(uint8_t len,  /* Number of bytes that will be used to calculate a PEC */
                    uint8_t* data /* Array of data that will be used to calculate  a PEC */
);

/**
 * @brief Compute the 10-bit data PEC (CRC-10) for one IC's register data.
 *
 * Polynomial x^10 + x^7 + x^3 + x^2 + x + 1 (0x08F), seed 16.
 *
 * @param[in] rx_cmd `true` when checking data *received* from the chip.
 *                   The chip also folds its 6-bit command counter (the top
 *                   6 bits of `data[len]`) into the CRC, so this flag tells
 *                   the function to include it. Use `false` when building
 *                   data to *send*.
 * @param[in] len    Number of data bytes (normally 6), NOT counting the
 *                   counter/PEC bytes.
 * @param[in] data   Data bytes. When @p rx_cmd is true, `data[len]` must also
 *                   be readable (it holds the command counter).
 * @return 10-bit PEC in the low bits (0x000–0x3FF).
 *
 * @note Pure function: no hardware access.
 */
uint16_t pec10_calc(bool rx_cmd, int len, uint8_t* data);

/**
 * @brief Send a command that has no data payload (e.g. ADCV, SNAP, CLRCELL).
 *
 * Builds the 4-byte frame (2 command bytes + PEC15), pulls chip-select low,
 * writes the frame, then releases chip-select.
 *
 * @param[in] tx_cmd 2 command bytes, e.g. one of the arrays in
 *                   adBms6830CmdList.h.
 *
 * Hardware calls (fake these in tests): adBmsCsLow(), spiWriteBytes(),
 * adBmsCsHigh().
 */
void spiSendCmd(uint8_t tx_cmd[2]);

/**
 * @brief Send a read command and receive one register group from every IC.
 *
 * Wakes the chain, sends the 4-byte command frame, then clocks in
 * `regData_size * tIC` bytes. For each IC it copies the raw bytes into
 * @p rx_data, pulls out the command counter, and compares the received
 * CRC-10 against one it computes itself.
 *
 * @param[in]  tIC          Number of ICs in the daisy chain.
 * @param[in]  tx_cmd       2 command bytes (e.g. `RDCVA`).
 * @param[out] rx_data      Buffer of at least `regData_size * tIC` bytes.
 *                          IC 0's bytes come first.
 * @param[out] pec_error    Array of @p tIC entries; set to 0 if that IC's
 *                          CRC matched, 1 if it did not.
 * @param[out] cmd_cntr     Array of @p tIC entries; each IC's 6-bit command
 *                          counter.
 * @param[in]  regData_size Bytes per IC, *including* the 2 PEC bytes
 *                          (8 for a normal group, e.g. 34 for RDCVALL).
 *
 * @warning Allocates two temporary buffers with calloc() and calls exit(0)
 *          if allocation fails.
 * @warning The internal buffer size is stored in a `uint8_t`, so
 *          `regData_size * tIC` must be <= 255. See @ref known_issues.
 *
 * Hardware calls: adBmsWakeupIc(), adBmsCsLow(), spiWriteReadBytes(),
 * adBmsCsHigh().
 */
void spiReadData(uint8_t tIC, uint8_t tx_cmd[2], uint8_t* rx_data, uint8_t* pec_error,
                 uint8_t* cmd_cntr, uint8_t regData_size);

/**
 * @brief Send a write command followed by 6 data bytes + CRC-10 for each IC.
 *
 * The frame is `4 + 8 * tIC` bytes. ICs are sent in reverse order (last IC
 * first) because the first chunk shifted into the chain lands in the IC
 * farthest from the MCU.
 *
 * @param[in] tIC    Number of ICs in the daisy chain.
 * @param[in] tx_cmd 2 command bytes (e.g. `WRCFGA`).
 * @param[in] data   `6 * tIC` bytes, IC 0's 6 bytes first.
 *
 * Hardware calls: adBmsCsLow(), spiWriteBytes(), adBmsCsHigh().
 */
void spiWriteData(uint8_t tIC, uint8_t tx_cmd[2], uint8_t* data);

/**
 * @brief High-level read: fetch a register group from all ICs and decode it
 *        into the @ref cell_asic array.
 *
 * Picks the right buffer size, calls spiReadData(), hands the raw bytes to
 * the matching `adBms6830Parse*()` function for @p type, and stores each IC's
 * PEC-error flag and command counter in `ic[n].cccrc`.
 *
 * @param[in]     tIC     Number of ICs in the daisy chain.
 * @param[in,out] ic      Array of @p tIC structs; results are written here
 *                        (e.g. `ic[n].cell.c_codes[]` for @c Cell).
 * @param[in]     cmd_arg 2 command bytes matching @p type and @p group
 *                        (e.g. `RDCVA` with `Cell, A`).
 * @param[in]     type    What is being read (@c Cell, @c Aux, @c Status,
 *                        @c Config, @c Sid, ...). See @ref TYPE.
 * @param[in]     group   Register group (@c A..@c F), @c ALL_GRP for the
 *                        "read all" commands, or @c NONE.
 *
 * @post `ic[n].cccrc.<type>_pec` is 0 if IC n's data passed its CRC check,
 *       1 otherwise. Each call *overwrites* the previous flag, so callers that
 *       read several groups must combine (OR) the flags themselves, as
 *       measurement_loop() does.
 *
 * @warning With @p group == @c ALL_GRP the buffer is sized for one IC only.
 *          See @ref known_issues before using the RD*ALL commands with more
 *          than one IC.
 */
void adBmsReadData(uint8_t tIC, cell_asic* ic, uint8_t cmd_arg[2], TYPE type, GRP group);

/**
 * @brief High-level write: encode settings from the @ref cell_asic array and
 *        write them to all ICs.
 *
 * Calls the matching `adBms6830Create*()` function to pack the `tx_*` fields
 * into 6 bytes per IC, then wakes the chain and calls spiWriteData().
 *
 * @param[in]     tIC     Number of ICs in the daisy chain.
 * @param[in,out] ic      Array of @p tIC structs. Reads `tx_cfga`, `tx_cfgb`,
 *                        `pwma`, ...; fills the matching `*.tx_data[]`.
 * @param[in]     cmd_arg 2 command bytes (e.g. `WRCFGA`).
 * @param[in]     type    @c Config, @c Comm, @c Pwm or @c Clrflag.
 * @param[in]     group   @c A or @c B (for @c Config and @c Pwm).
 */
void adBmsWriteData(uint8_t tIC, cell_asic* ic, uint8_t cmd_arg[2], TYPE type, GRP group);

/**
 * @brief Send an ADC command and busy-wait until the conversion finishes.
 *
 * Only compiled when `TIM_EN` is 1 (it is 0 by default, so this function is
 * not currently built).
 *
 * @param[in] tx_cmd 2 command bytes of a poll command (e.g. `PLADC`).
 * @return Conversion time in timer counts.
 */
uint32_t adBmsPollAdc(uint8_t tx_cmd[2]);

/**
 * @brief Start a cell-voltage (C-ADC) conversion (ADCV command).
 *
 * Packs the options into the 2 command bytes and sends them with
 * spiSendCmd():
 * @code
 *   cmd[0] = 0x02 + rd
 *   cmd[1] = (cont << 7) | (dcp << 4) | (rstf << 2) | (owcs & 0x03) | 0x60
 * @endcode
 *
 * @param[in] rd   @c RD_ON also starts the redundant S-ADC.
 * @param[in] cont @c CONTINUOUS keeps converting; @c SINGLE converts once.
 * @param[in] dcp  @c DCP_ON allows cell discharge during the measurement.
 * @param[in] rstf @c RSTF_ON resets the IIR filter.
 * @param[in] owcs Open-wire test switches (@c OW_OFF_ALL_CH for normal use).
 */
void adBms6830_Adcv(RD rd, CONT cont, DCP dcp, RSTF rstf, OW_C_S owcs);

/**
 * @brief Start an S-ADC (redundant cell) conversion (ADSV command).
 *
 * @param[in] cont @c CONTINUOUS or @c SINGLE.
 * @param[in] dcp  Discharge permitted during measurement.
 * @param[in] owcs Open-wire test switches.
 */
void adBms6830_Adsv(CONT cont, DCP dcp, OW_C_S owcs);

/**
 * @brief Start an auxiliary (GPIO / temperature) conversion (ADAX command).
 *
 * The pack's thermistor/temperature sensors are wired to the GPIO inputs,
 * so this is how temperatures get measured.
 *
 * @param[in] owaux @c AUX_OW_ON to run the aux open-wire test.
 * @param[in] pup   Pull-up/down current for the open-wire test.
 * @param[in] ch    Channel to convert, or @c AUX_ALL for all of them.
 */
void adBms6830_Adax(OW_AUX owaux, PUP pup, CH ch);

/**
 * @brief Start a redundant auxiliary conversion (ADAX2 command).
 *
 * @param[in] ch Channel to convert, or @c AUX_ALL.
 */
void adBms6830_Adax2(CH ch);

/**
 * @brief Freeze the result registers (SNAP) so a consistent set can be read
 *        while the ADC keeps running in continuous mode.
 * @see adBms6830_Unsnap()
 */
void adBms6830_Snap();

/**
 * @brief Release the result registers frozen by adBms6830_Snap().
 */
void adBms6830_Unsnap();

#endif
/** @}*/
/** @}*/
