/*******************************************************************************
Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensor.
******************************************************************************
* @file:    mcuWrapper.h
* @brief:   Generic wrapper header file
* @version: $Revision$
* @date:    $Date$
* Developed by: ADIBMS Software team, Bangalore, India
*****************************************************************************/

/**
 * @file mcuWrapper.h
 * @brief Hardware abstraction layer: the ONLY place the ADBMS6830 driver
 *        touches the STM32 (SPI bus, chip-select pin, delays).
 *
 * Every function here is a thin wrapper around an STM32 HAL call. The rest
 * of the driver never calls the HAL directly, which means:
 * - porting to another microcontroller only means rewriting mcuWrapper.c, and
 * - in unit tests you can replace these functions with fakes (fff) and run
 *   the whole protocol layer on a laptop with no hardware.
 *
 * @par Current wiring (from mcuWrapper.c / main.c)
 * - SPI: `hspi1` (SPI1), 500 ms timeout per transfer.
 * - Chip select: GPIOB pin 6 (`CS_PIN` / `GPIO_PORT`), active low.
 *
 * @ingroup MUC_DRIVER
 */

/** @addtogroup MUC_DRIVER
 *  @{
 *
 */

/** @addtogroup SPI_DRIVER SPI DRIVER
 *  @{
 *
 */
#ifndef __ADBMSWRAPPER_H
#define __ADBMSWRAPPER_H
#include "common.h"

#ifdef MBED

#else
#include "main.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_it.h"

// extern ADC_HandleTypeDef hadc1;         /* Mcu dependent ADC handler */
// extern I2C_HandleTypeDef hi2c1;         /* Mcu dependent I2C handler */
// extern I2C_HandleTypeDef hi2c3;         /* Mcu dependent I2C handler */
extern SPI_HandleTypeDef hspi1;   /* Mcu dependent SPI handler */
extern SPI_HandleTypeDef hspi5;   /* Mcu dependent SPI handler */
extern UART_HandleTypeDef huart4; /* Mcu dependent UART handler */
extern UART_HandleTypeDef huart5; /* Mcu dependent UART handler */

#define CS_PIN GPIO_PIN_6 /* Mcu dependent chip select */
#define GPIO_PORT GPIOB   /* Mcu dependent adc chip select port */
#endif

/**
 * @brief Block for @p delay milliseconds (wraps `HAL_Delay`).
 * @param[in] delay Time to wait, in ms.
 */
void Delay_ms(uint32_t delay);

/**
 * @brief Pull the ADBMS chip-select line LOW (start of an SPI transaction).
 *
 * Wraps `HAL_GPIO_WritePin(GPIO_PORT, CS_PIN, GPIO_PIN_RESET)`.
 * Every transaction must be wrapped as adBmsCsLow() ... adBmsCsHigh().
 */
void adBmsCsLow(void);

/**
 * @brief Pull the ADBMS chip-select line HIGH (end of an SPI transaction).
 */
void adBmsCsHigh(void);

/**
 * @brief Transmit @p size bytes on the SPI bus (wraps `HAL_SPI_Transmit`).
 *
 * Does NOT touch chip-select; the caller does that.
 *
 * @param[in] size    Number of bytes to send.
 * @param[in] tx_Data Bytes to send.
 */
void spiWriteBytes(uint16_t size,   /*Option: Number of bytes to be written on the SPI port*/
                   uint8_t* tx_Data /*Array of bytes to be written on the SPI port*/
);
/**
 * @brief Send a 4-byte command frame, then receive @p size bytes.
 *
 * On STM32 this is `HAL_SPI_Transmit(4 bytes)` followed by
 * `HAL_SPI_Receive(size bytes)`.
 *
 * @param[in]  tx_data Command frame. Only the first 4 bytes are sent.
 * @param[out] rx_data Buffer of at least @p size bytes for the reply.
 * @param[in]  size    Number of bytes to receive (NOT counting the 4 sent).
 */
void spiWriteReadBytes(
    uint8_t* tx_data, /*array of data to be written on SPI port*/
    uint8_t* rx_data, /*Input: array that will store the data read by the SPI port*/
    uint16_t size     /*Option: number of bytes*/
);
/**
 * @brief Receive @p size bytes from the SPI bus (wraps `HAL_SPI_Receive`).
 * @param[in]  size    Number of bytes to receive.
 * @param[out] rx_data Buffer of at least @p size bytes.
 */
void spiReadBytes(uint16_t size, uint8_t* rx_data);

/** @brief Start the conversion timer. Only built when `TIM_EN` is 1 (off by default). */
void startTimer(void);
/** @brief Stop the conversion timer. Only built when `TIM_EN` is 1. */
void stopTimer(void);
/** @brief Read and reset the conversion timer. Only built when `TIM_EN` is 1.
 *  @return Timer count. */
uint32_t getTimCount(void);

/**
 * @brief Wake every chip in the daisy chain from sleep/idle.
 *
 * The chips go to sleep when the bus is quiet. Toggling chip-select once per
 * chip (low 1 ms, high 1 ms) wakes them one after another down the chain, so
 * this takes about `2 * total_ic` ms.
 *
 * @param[in] total_ic Number of chips in the daisy chain.
 *
 * @note Call this before any command if the bus may have been idle.
 *       spiReadData() and adBmsWriteData() already call it for you.
 */
void adBmsWakeupIc(uint8_t total_ic);

#endif
/** @}*/
/** @}*/
