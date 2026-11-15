/**
 * @file stm32g4xx_hal.h
 * @brief Host-test stand-in for the STM32 HAL: only the types that
 *        mcuWrapper.h mentions in its extern declarations.
 */
#ifndef TEST_STUB_STM32G4XX_HAL_H
#define TEST_STUB_STM32G4XX_HAL_H
#include <stdint.h>

typedef struct {
    int unused;
} SPI_HandleTypeDef;

typedef struct {
    int unused;
} UART_HandleTypeDef;

#endif
