/**
 * @file test_helpers.h
 * @brief Small helpers shared by the ADBMS6830 tests.
 */
#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdint.h>

#include "adbms_main.h"

/**
 * @brief Build one IC's 8-byte read reply exactly like a real ADBMS6830.
 *
 * Layout: [d0..d5][counter<<2 | PEC[9:8]][PEC[7:0]].
 * The PEC covers the 6 data bytes AND the 6-bit counter, which is why the
 * counter goes into byte 6 *before* pec10_calc() runs.
 *
 * @param[out] out     8-byte buffer to fill.
 * @param[in]  data6   The 6 data bytes.
 * @param[in]  counter Command counter (0-63).
 */
static inline void make_ic_reply(uint8_t out[RX_DATA], const uint8_t data6[TX_DATA],
                                 uint8_t counter) {
    for (int i = 0; i < TX_DATA; i++) {
        out[i] = data6[i];
    }
    out[6] = (uint8_t)(counter << 2);
    uint16_t pec = pec10_calc(true, TX_DATA, out);
    out[6] |= (uint8_t)((pec >> 8) & 0x03);
    out[7] = (uint8_t)(pec & 0xFF);
}

#endif
