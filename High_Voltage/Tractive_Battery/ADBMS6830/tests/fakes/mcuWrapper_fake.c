/**
 * @file mcuWrapper_fake.c
 * @brief Definitions of the fakes declared in mcuWrapper_fake.h.
 */
#include "mcuWrapper_fake.h"

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VOID_FUNC(Delay_ms, uint32_t);
DEFINE_FAKE_VOID_FUNC(adBmsCsLow);
DEFINE_FAKE_VOID_FUNC(adBmsCsHigh);
DEFINE_FAKE_VOID_FUNC(spiWriteBytes, uint16_t, uint8_t*);
DEFINE_FAKE_VOID_FUNC(spiWriteReadBytes, uint8_t*, uint8_t*, uint16_t);
DEFINE_FAKE_VOID_FUNC(spiReadBytes, uint16_t, uint8_t*);
DEFINE_FAKE_VOID_FUNC(startTimer);
DEFINE_FAKE_VOID_FUNC(stopTimer);
DEFINE_FAKE_VALUE_FUNC(uint32_t, getTimCount);
DEFINE_FAKE_VOID_FUNC(adBmsWakeupIc, uint8_t);

void mcuWrapper_fakes_reset(void) {
    RESET_FAKE(Delay_ms);
    RESET_FAKE(adBmsCsLow);
    RESET_FAKE(adBmsCsHigh);
    RESET_FAKE(spiWriteBytes);
    RESET_FAKE(spiWriteReadBytes);
    RESET_FAKE(spiReadBytes);
    RESET_FAKE(startTimer);
    RESET_FAKE(stopTimer);
    RESET_FAKE(getTimCount);
    RESET_FAKE(adBmsWakeupIc);
    FFF_RESET_HISTORY();
}
