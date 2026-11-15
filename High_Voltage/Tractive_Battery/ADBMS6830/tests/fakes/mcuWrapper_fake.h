/**
 * @file mcuWrapper_fake.h
 * @brief fff fakes for every function in mcuWrapper.h.
 *
 * The real mcuWrapper.c calls the STM32 HAL, which can't run on a laptop.
 * The test build links mcuWrapper_fake.c instead. Each fake does nothing
 * except remember how it was called, e.g.:
 *   - `adBmsCsLow_fake.call_count`      how many times it ran
 *   - `spiWriteBytes_fake.arg0_val`     the `size` argument of the last call
 *   - `fff.call_history[i]`             which fake ran i-th (call order)
 *
 * Tests can also give a fake a `custom_fake` function to run instead, which
 * is how we capture the bytes sent and inject the bytes "received".
 */
#ifndef MCUWRAPPER_FAKE_H
#define MCUWRAPPER_FAKE_H

#include <stdint.h>

#include "fff.h"

DECLARE_FAKE_VOID_FUNC(Delay_ms, uint32_t);
DECLARE_FAKE_VOID_FUNC(adBmsCsLow);
DECLARE_FAKE_VOID_FUNC(adBmsCsHigh);
DECLARE_FAKE_VOID_FUNC(spiWriteBytes, uint16_t, uint8_t*);
DECLARE_FAKE_VOID_FUNC(spiWriteReadBytes, uint8_t*, uint8_t*, uint16_t);
DECLARE_FAKE_VOID_FUNC(spiReadBytes, uint16_t, uint8_t*);
DECLARE_FAKE_VOID_FUNC(startTimer);
DECLARE_FAKE_VOID_FUNC(stopTimer);
DECLARE_FAKE_VALUE_FUNC(uint32_t, getTimCount);
DECLARE_FAKE_VOID_FUNC(adBmsWakeupIc, uint8_t);

/** Reset every fake's counters/arguments and the shared call history.
 *  Call this from setUp() so tests can't affect each other. */
void mcuWrapper_fakes_reset(void);

#endif
