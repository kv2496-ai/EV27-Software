/*******************************************************************************
Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensor.
******************************************************************************
* @file:    adBms_Application.h
* @brief:   Bms application header file
* @version: $Revision$
* @date:    $Date$
* Developed by: ADIBMS Software team, Bangalore, India
*****************************************************************************/

/**
 * @file adBms_Application.h
 * @brief EV27 BMS application layer: pack configuration, the main init/run
 *        entry points, measurement loop, and SOC (state of charge).
 *
 * This is the team-written layer on top of the ADI driver. It owns the
 * global `cell_asic IC[TOTAL_IC]` array and turns raw chip readings into pack
 * data (`PackSegments[]`, `TotalPack`) that the CAN code sends out.
 *
 * @par Typical call order (from main.c)
 * @code
 *   adbms_main_init(&can_data);          // once, at boot
 *   while (1) {
 *       adBms_main_run(&can_data, &gpio); // every loop
 *   }
 * @endcode
 *
 * @par Pack layout
 * The full pack is 6 segments, each read by 2 chips (a "master" with 14
 * cells and a "slave" with 10 cells, 24 cells per segment). The code is
 * currently set up for a partial/test-bench pack: `TOTAL_IC` = 6 chips and
 * `CELLS_PER_MOD` = 10 cells each (see the TODOs in adBms_Application.c).
 *
 * @ingroup APPLICATION
 */

/*! @addtogroup APPLICATION
 *  @{
 *
 */

/*! @addtogroup APPLICATION
 *  @{
 *
 */

#ifndef __APPLICATION_H
#define __APPLICATION_H

#include <stdint.h>

#include "adbms_main.h"
#include "common_types.h"
#include "kalman_soc.h"

/** Number of ADBMS6830 chips in the daisy chain (size of the global IC[] array). */
#define TOTAL_IC 6

// SOC CHANGES
/** Voltage tolerance (V) used when comparing cells in the SOC code. */
#define CELL_TIE_TOLERANCE 0.0002f
#define TOTAL_MODULES 6 /**< Number of battery segments (full pack: 2 chips per segment). */
#define TOTAL_CELLS TOTAL_MODULES* CELLS_PER_MOD /**< Cells monitored in total. */
#define MIN_CELL_THRESH                                                      \
    0.2 /**< Minimum fraction of working temperature sensors before a fault. \
         */
#define SENSOR_DROPOUT_TEMP -99 /**< Temperature value meaning "sensor not connected". */
#define TEMP_PER_BOARD 10       /**< Temperature sensors read per chip. */
#define VOLT_PER_SLAVE 10       /**< Cells on a slave chip. */
#define VOLT_PER_MASTER 14      /**< Cells on a master chip. */
/** Consecutive bad-PEC cycles before a segment is flagged as a comm fault. */
#define COMM_FAULT_LATCH 3

/** @name Board serial IDs
 *  48-bit chip serial IDs used by lookupID() to identify which physical
 *  board each chip is on. Update these when a board is replaced.
 *  @{ */
#define ID1_TOP 0x97DE40006B21ULL
#define ID2_TOP 0x997CE40586B2ULL
#define ID3_TOP 0x097EE40006B2ULL
#define ID4_TOP 0x990DE40686B2ULL
#define ID5_TOP 0xC97DE40506B2ULL
#define ID6_TOP 0x590EE40586B2ULL
#define ID7_TOP 0x9A06E40606B2ULL
#define ID1_BOTTOM 0x0001111111ULL
#define ID2_BOTTOM 0x0002111111ULL
#define ID3_BOTTOM 0x0003111111ULL
#define ID4_BOTTOM 0x0004111111ULL
#define ID5_BOTTOM 0x0005111111ULL
#define ID6_BOTTOM 0x0006111111ULL
#define ID7_BOTTOM 0x0001111111ULL
/** @} */

/** @name ADI example/diagnostic functions
 *  Kept from Analog Devices' reference application. Mostly used for bench
 *  testing via run_command(); the EV27 main loop does not call most of them.
 *  @{ */
void app_main(void);
void run_command(int cmd);
void adBms6830_init_config(uint8_t tIC, cell_asic* ic);
void adBms6830_write_read_config(uint8_t tIC, cell_asic* ic);
void adBms6830_write_config(uint8_t tIC, cell_asic* ic);
void adBms6830_read_config(uint8_t tIC, cell_asic* ic);
void adBms6830_start_adc_cell_voltage_measurment(uint8_t tIC);
void adBms6830_read_cell_voltages(uint8_t tIC, cell_asic* ic);
void adBms6830_start_adc_s_voltage_measurment(uint8_t tIC);
void adBms6830_read_s_voltages(uint8_t tIC, cell_asic* ic);
void adBms6830_start_avgcell_voltage_measurment(uint8_t tIC);
void adBms6830_read_avgcell_voltages(uint8_t tIC, cell_asic* ic);
void adBms6830_start_fcell_voltage_measurment(uint8_t tIC);
void adBms6830_read_fcell_voltages(uint8_t tIC, cell_asic* ic);
void adBms6830_start_aux_voltage_measurment(uint8_t tIC, cell_asic* ic);
void adBms6830_read_aux_voltages(uint8_t tIC, cell_asic* ic);
void adBms6830_start_raux_voltage_measurment(uint8_t tIC, cell_asic* ic);
void adBms6830_read_raux_voltages(uint8_t tIC, cell_asic* ic);
void adBms6830_read_status_registers(uint8_t tIC, cell_asic* ic);
/**
 * @brief Read every cell voltage and temperature, then update pack data.
 *
 * 1. SNAP (freeze results), read cell groups A-F, UNSNAP. The PEC error
 *    flags from all 6 reads are OR-ed together so a bad read of any group is
 *    caught.
 * 2. Start an aux conversion (ADAX), wait 5 ms, read aux groups A-D
 *    (temperatures), again OR-ing the PEC flags.
 * 3. Start a redundant aux conversion (ADAX2), wait 5 ms, read RAUX A-D.
 * 4. Process_Board_Data(): convert codes to mV / deg C, set OV/UV/OT/comm
 *    fault bits in `PackSegments[]`.
 * 5. If `can_data->balancing_enable`, run cell balancing.
 *
 * @param[in] can_data CAN inputs.
 */
void measurement_loop(volatile CAN_Inputs_t* can_data);
void adBms6830_read_device_sid(uint8_t tIC, cell_asic* ic);
void adBms6830_set_reset_gpio_pins(uint8_t tIC, cell_asic* ic);
void adBms6830_enable_mute(uint8_t tIC, cell_asic* ic);
void adBms6830_disable_mute(uint8_t tIC, cell_asic* ic);
void adBms6830_soft_reset(uint8_t tIC);
void adBms6830_reset_cmd_count(uint8_t tIC);
void adBms6830_reset_pec_error_flag(uint8_t tIC, cell_asic* ic);
void adBms6830_snap(uint8_t tIC);
void adBms6830_unsnap(uint8_t tIC);
void adBms6830_clear_cell_measurement(uint8_t tIC);
void adBms6830_clear_aux_measurement(uint8_t tIC);
void adBms6830_clear_spin_measurement(uint8_t tIC);
void adBms6830_clear_fcell_measurement(uint8_t tIC);
void adBms6830_clear_ovuv_measurement(uint8_t tIC);
void adBms6830_clear_all_flags(uint8_t tIC, cell_asic* ic);
void adBms6830_set_dcc_discharge(uint8_t tIC, cell_asic* ic);
void adBms6830_clear_dcc_discharge(uint8_t tIC, cell_asic* ic);
void adBms6830_write_read_pwm_duty_cycle(uint8_t tIC, cell_asic* ic);
void adBms6830_gpio_spi_communication(uint8_t tIC, cell_asic* ic);
void adBms6830_gpio_i2c_write_to_slave(uint8_t tIC, cell_asic* ic);
void adBms6830_gpio_i2c_read_from_slave(uint8_t tIC, cell_asic* ic);
void adBms6830_set_dtrng_dcto_value(uint8_t tIC, cell_asic* ic);
void adBms6830_run_osc_mismatch_self_test(uint8_t tIC, cell_asic* ic);
void adBms6830_run_thermal_shutdown_self_test(uint8_t tIC, cell_asic* ic);
void adBms6830_run_supply_error_detection_self_test(uint8_t tIC, cell_asic* ic);
void adBms6830_run_thermal_shutdown_self_test(uint8_t tIC, cell_asic* ic);
void adBms6830_run_fuse_ed_self_test(uint8_t tIC, cell_asic* ic);
void adBms6830_run_fuse_med_self_test(uint8_t tIC, cell_asic* ic);
void adBms6830_run_tmodchk_self_test(uint8_t tIC, cell_asic* ic);
void adBms6830_check_latent_fault_csflt_status_bits(uint8_t tIC, cell_asic* ic);
void adBms6830_check_rdstatc_err_bit_functionality(uint8_t tIC, cell_asic* ic);
void adBms6830_cell_openwire_test(uint8_t tIC, cell_asic* ic);
void adBms6830_redundant_cell_openwire_test(uint8_t tIC, cell_asic* ic);
void adBms6830_cell_ow_volatge_collect(uint8_t tIC, cell_asic* ic, TYPE type, OW_C_S ow_c_s);
void adBms6830_aux_openwire_test(uint8_t tIC, cell_asic* ic);
void adBms6830_gpio_pup_up_down_volatge_collect(uint8_t tIC, cell_asic* ic, PUP pup);
void adBms6830_open_wire_detection_condtion_check(uint8_t tIC, cell_asic* ic, TYPE type);
void adBms6830_read_rdcvall_voltage(uint8_t tIC, cell_asic* ic);
void adBms6830_read_rdacall_voltage(uint8_t tIC, cell_asic* ic);
void adBms6830_read_rdsall_voltage(uint8_t tIC, cell_asic* ic);
void adBms6830_read_rdfcall_voltage(uint8_t tIC, cell_asic* ic);
void adBms6830_read_rdcsall_voltage(uint8_t tIC, cell_asic* ic);
void adBms6830_read_rdacsall_voltage(uint8_t tIC, cell_asic* ic);
void adBms6830_read_rdasall_voltage(uint8_t tIC, cell_asic* ic);
/** @} */

/** @name Main entry points
 *  @{ */

/**
 * @brief Configure all chips and take the first measurement. Call once at boot.
 *
 * Steps:
 * 1. Set each chip's config: reference on, all GPIOs released,
 *    OV/UV thresholds from `OV_THRESHOLD` (4.25 V) / `UV_THRESHOLD` (2.5 V).
 * 2. Write Config A, Config B, PWM A and PWM B to every chip.
 * 3. Start the cell ADC in continuous mode (ADCV) and wait 8 ms.
 * 4. Read each chip's serial ID and map it to a board number with lookupID().
 * 5. Run measurement_loop() once, then initialise the SOC estimator.
 *
 * @param[in] can_data CAN inputs (passed through to measurement_loop()).
 */
void adbms_main_init(volatile CAN_Inputs_t* can_data);

/**
 * @brief One pass of the BMS main loop. main.c calls it every 1000 ms.
 *
 * Checks for the charger, runs measurement_loop(), updates the BMS-OK output
 * pin from the fault flags, then updates SOC.
 *
 * @param[in] can_data  CAN inputs (e.g. balancing enable).
 * @param[in] gpio_data GPIO state used to drive the BMS fault output.
 */
void adBms_main_run(volatile CAN_Inputs_t* can_data, GPIO_Info_t* gpio_data);
/** @} */

// SOC Functions
void adBms6830_soc_run(volatile CAN_Inputs_t* can_data);
void adBms6830_soc_init(void);
void adBms6830_soc_update(int cic, int cell, float cell_voltage, float pack_current,
                          float cell_temp);
void adBms6830_print_soc_estimate(int cic, int cell, const SOC_Estimate* est);
void adBms6830_soc_save_to_flash(void);
void adBms6830_soc_clear_flash(void);

/** @name Helpers
 *  @{ */
/** @brief Map a cell index to the temperature sensor index that covers it. */
int cell_to_temp_index(int cell_i);
/** @brief Map a 48-bit chip serial ID to a board number (1-14).
 *  @param[in] id Serial ID assembled from `IC[n].sid.sid[0..5]`.
 *  @return Board number: 1-7 = TOP boards, 8-14 = BOTTOM boards,
 *          0 if the ID is not in the table. */
int lookupID(uint64_t id);
/** @} */

/** @name Global pack state (defined in adBms_Application.c)
 *  @{ */
extern SegmentData_t
    PackSegments[TOTAL_MODULES]; /**< Per-segment voltages (mV), temps, fault flags. */
extern TotalPack_t TotalPack;    /**< Whole-pack voltage, average temp, balancing state. */
extern SOC_Estimate g_soc_estimate[TOTAL_MODULES][CELLS_PER_MOD]; /**< Latest SOC per cell. */
/** @} */

#endif
/** @}*/
/** @}*/
