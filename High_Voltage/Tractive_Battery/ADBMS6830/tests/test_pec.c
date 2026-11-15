/**
 * @file test_pec.c
 * @brief Tests for the PEC (CRC) functions: Pec15_Calc() and pec10_calc().
 *
 * These are pure functions, so no fakes are needed: give an input, check the
 * output. The PEC is what lets the BMS notice corrupted SPI messages, so if
 * these are wrong, every read would look corrupted (or worse, no corruption
 * would ever be caught).
 */
#include "adbms_main.h"
#include "mcuWrapper_fake.h"
#include "unity.h"

void setUp(void) { mcuWrapper_fakes_reset(); }
void tearDown(void) {}

/* ---------- Pec15_Calc (command PEC) ---------- */

/* Known answer: the RDCVA command {0x00, 0x04} has PEC 0x07C2. This is the
 * worked example used in ADI's battery-monitor datasheets. If this fails,
 * the CRC table or seed is wrong. */
void test_pec15_rdcva_known_value(void) {
    uint8_t cmd[2] = {0x00, 0x04};
    TEST_ASSERT_EQUAL_HEX16(0x07C2, Pec15_Calc(2, cmd));
}

void test_pec15_wrcfga_known_value(void) {
    uint8_t cmd[2] = {0x00, 0x01};
    TEST_ASSERT_EQUAL_HEX16(0x3D6E, Pec15_Calc(2, cmd));
}

/* The 15-bit PEC is returned shifted left by 1, so bit 0 is always 0. */
void test_pec15_lowest_bit_is_always_zero(void) {
    uint8_t cmd[2];
    for (uint16_t c = 0; c < 0x800; c++) { /* every 11-bit command code */
        cmd[0] = (uint8_t)(c >> 8);
        cmd[1] = (uint8_t)c;
        TEST_ASSERT_EQUAL_UINT16(0, Pec15_Calc(2, cmd) & 0x0001);
    }
}

/* The whole point of a CRC: flipping ANY single bit must change the PEC. */
void test_pec15_detects_every_single_bit_flip(void) {
    uint8_t cmd[2] = {0x00, 0x04};
    uint16_t good = Pec15_Calc(2, cmd);
    for (int bit = 0; bit < 16; bit++) {
        uint8_t bad[2] = {cmd[0], cmd[1]};
        bad[bit / 8] ^= (uint8_t)(1u << (bit % 8));
        TEST_ASSERT_NOT_EQUAL_MESSAGE(good, Pec15_Calc(2, bad), "bit flip not detected");
    }
}

/* Pure function: same input, same output, and the input is not modified. */
void test_pec15_does_not_modify_input(void) {
    uint8_t cmd[2] = {0x12, 0x34};
    (void)Pec15_Calc(2, cmd);
    TEST_ASSERT_EQUAL_HEX8(0x12, cmd[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34, cmd[1]);
}

/* ---------- pec10_calc (data PEC) ---------- */

void test_pec10_result_fits_in_10_bits(void) {
    uint8_t data[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    TEST_ASSERT_LESS_OR_EQUAL_HEX16(0x3FF, pec10_calc(false, 6, data));
    TEST_ASSERT_LESS_OR_EQUAL_HEX16(0x3FF, pec10_calc(true, 6, data));
}

void test_pec10_detects_every_single_bit_flip_in_data(void) {
    uint8_t data[6] = {0xBC, 0x34, 0x10, 0x27, 0x00, 0x80};
    uint16_t good = pec10_calc(false, 6, data);
    for (int bit = 0; bit < 48; bit++) {
        uint8_t bad[6];
        memcpy(bad, data, 6);
        bad[bit / 8] ^= (uint8_t)(1u << (bit % 8));
        TEST_ASSERT_NOT_EQUAL_MESSAGE(good, pec10_calc(false, 6, bad), "bit flip not detected");
    }
}

/* On received data the chip also protects its command counter (top 6 bits
 * of byte 6). With a counter of 0 that adds nothing, so rx == tx. */
void test_pec10_rx_with_zero_counter_equals_tx(void) {
    uint8_t data[7] = {1, 2, 3, 4, 5, 6, 0x00};
    TEST_ASSERT_EQUAL_HEX16(pec10_calc(false, 6, data), pec10_calc(true, 6, data));
}

void test_pec10_rx_includes_command_counter(void) {
    uint8_t a[7] = {1, 2, 3, 4, 5, 6, (uint8_t)(1 << 2)}; /* counter = 1 */
    uint8_t b[7] = {1, 2, 3, 4, 5, 6, (uint8_t)(2 << 2)}; /* counter = 2 */
    TEST_ASSERT_NOT_EQUAL(pec10_calc(true, 6, a), pec10_calc(true, 6, b));
}

/* Byte 6's low 2 bits are the PEC itself, so they must NOT feed the PEC. */
void test_pec10_rx_ignores_low_two_bits_of_counter_byte(void) {
    uint8_t a[7] = {1, 2, 3, 4, 5, 6, 0x14};
    uint8_t b[7] = {1, 2, 3, 4, 5, 6, 0x17};
    TEST_ASSERT_EQUAL_HEX16(pec10_calc(true, 6, a), pec10_calc(true, 6, b));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pec15_rdcva_known_value);
    RUN_TEST(test_pec15_wrcfga_known_value);
    RUN_TEST(test_pec15_lowest_bit_is_always_zero);
    RUN_TEST(test_pec15_detects_every_single_bit_flip);
    RUN_TEST(test_pec15_does_not_modify_input);
    RUN_TEST(test_pec10_result_fits_in_10_bits);
    RUN_TEST(test_pec10_detects_every_single_bit_flip_in_data);
    RUN_TEST(test_pec10_rx_with_zero_counter_equals_tx);
    RUN_TEST(test_pec10_rx_includes_command_counter);
    RUN_TEST(test_pec10_rx_ignores_low_two_bits_of_counter_byte);
    return UNITY_END();
}
