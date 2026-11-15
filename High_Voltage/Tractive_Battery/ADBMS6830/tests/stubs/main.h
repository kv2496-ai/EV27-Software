/**
 * @file main.h
 * @brief Host-test stand-in for the STM32CubeMX main.h.
 *
 * The driver headers include main.h / stm32g4xx_hal.h for the STM32 types.
 * On a laptop those don't exist, so the test build puts this stubs/ folder
 * first on the include path and these empty versions are used instead.
 */
#ifndef TEST_STUB_MAIN_H
#define TEST_STUB_MAIN_H
#include "stm32g4xx_hal.h"
#endif
