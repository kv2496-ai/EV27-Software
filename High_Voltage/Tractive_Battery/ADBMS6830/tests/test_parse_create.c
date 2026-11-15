/**
 * @file test_parse_create.c
 * @brief Tests for adBms6830ParseCreate.c: turning raw bytes into struct
 *        fields (Parse), struct fields into bytes (Create), and the value
 *        helpers (thresholds, bit masks).
 *
 * All of these are pure functions: they only read and write memory. Each test
 * hand-builds the bytes a chip would send, calls the function, and checks
 * the fields. No fakes needed.
 */
#include <string.h>

#include "adbms_main.h"
#include "mcuWrapper_fake.h"
#include "unity.h"

/* Real driver code keeps cell_asic arrays as globals. Use a fresh, zeroed
 * array for every test so one test can't leak values into the next. */
static cell_asic ic[2];

void setUp(void) {
    memset(ic, 0, sizeof(ic));
    mcuWrapper_fakes_reset();
}
void tearDown(void) {}

/* ---------- Value helpers ---------- */

/* (4.25 - 1.5) / 2.4 mV = 1145.8 -> truncated to 1145 */
void test_overvoltage_threshold_4v25(void) {
    TEST_ASSERT_EQUAL_UINT16(1145, SetOverVoltageThreshold(4.25f));
}

/* (2.5 - 1.5) / 2.4 mV = 416.7 -> 416 */
void test_undervoltage_threshold_2v5(void) {
    TEST_ASSERT_EQUAL_UINT16(416, SetUnderVoltageThreshold(2.5f));
}

void test_thresholds_fit_in_12_bits(void) {
    TEST_ASSERT_LESS_OR_EQUAL_UINT16(0x0FFF, SetOverVoltageThreshold(5.0f));
    TEST_ASSERT_LESS_OR_EQUAL_UINT16(0x0FFF, SetUnderVoltageThreshold(1.0f));
}

void test_configA_flag_set_and_clear(void) {
    TEST_ASSERT_EQUAL_HEX8(0x01, ConfigA_Flag(FLAG_D0, FLAG_SET));
    TEST_ASSERT_EQUAL_HEX8(0x80, ConfigA_Flag(FLAG_D7, FLAG_SET));
    TEST_ASSERT_EQUAL_HEX8(0x00, ConfigA_Flag(FLAG_D7, FLAG_CLR));
}

void test_dcc_bit_for_each_cell(void) {
    TEST_ASSERT_EQUAL_HEX16(0x0001, ConfigB_DccBit(DCC1, DCC_BIT_SET));
    TEST_ASSERT_EQUAL_HEX16(0x8000, ConfigB_DccBit(DCC16, DCC_BIT_SET));
    TEST_ASSERT_EQUAL_HEX16(0x0000, ConfigB_DccBit(DCC16, DCC_BIT_CLR));
}

void test_set_pwm_duty_cycle_sets_every_channel_of_every_ic(void) {
    SetPwmDutyCycle(2, ic, PWM_26_4_PCT);
    for (int n = 0; n < 2; n++) {
        for (int k = 0; k < PWMA; k++) {
            TEST_ASSERT_EQUAL_UINT8(PWM_26_4_PCT, ic[n].PwmA.pwma[k]);
        }
        for (int k = 0; k < PWMB; k++) {
            TEST_ASSERT_EQUAL_UINT8(PWM_26_4_PCT, ic[n].PwmB.pwmb[k]);
        }
    }
}

/* ---------- Cell voltage parsing ---------- */

/* 13500 = 0x34BC arrives low byte first: BC 34. 13500 * 150 uV + 1.5 V = 3.525 V */
void test_parse_cell_group_A_is_little_endian(void) {
    uint8_t rx[8] = {0xBC, 0x34, 0x10, 0x27, 0x00, 0x00, 0, 0};
    adBms6830ParseCell(1, ic, A, rx);
    TEST_ASSERT_EQUAL_INT16(13500, ic[0].cell.c_codes[0]);
    TEST_ASSERT_EQUAL_INT16(10000, ic[0].cell.c_codes[1]); /* 0x2710 */
    TEST_ASSERT_EQUAL_INT16(0, ic[0].cell.c_codes[2]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 3.525, ic[0].cell.c_codes[0] * 0.000150 + 1.5);
}

void test_parse_cell_each_group_fills_the_right_cells(void) {
    uint8_t rx[8] = {0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0, 0};
    GRP groups[] = {A, B, C, D, E};
    for (int g = 0; g < 5; g++) {
        memset(ic, 0, sizeof(ic));
        adBms6830ParseCell(1, ic, groups[g], rx);
        int first = g * 3;
        TEST_ASSERT_EQUAL_INT16(1, ic[0].cell.c_codes[first]);
        TEST_ASSERT_EQUAL_INT16(2, ic[0].cell.c_codes[first + 1]);
        TEST_ASSERT_EQUAL_INT16(3, ic[0].cell.c_codes[first + 2]);
    }
}

/* Group F only has cell 16; it must not touch anything else. */
void test_parse_cell_group_F_fills_only_cell_16(void) {
    uint8_t rx[8] = {0x34, 0x12, 0x99, 0x99, 0x99, 0x99, 0, 0};
    adBms6830ParseCell(1, ic, F, rx);
    TEST_ASSERT_EQUAL_HEX16(0x1234, ic[0].cell.c_codes[15]);
    for (int k = 0; k < 15; k++) {
        TEST_ASSERT_EQUAL_INT16(0, ic[0].cell.c_codes[k]);
    }
}

/* Codes are signed: 0xFFFF must become -1, not 65535. */
void test_parse_cell_handles_negative_codes(void) {
    uint8_t rx[8] = {0xFF, 0xFF, 0x00, 0x80, 0, 0, 0, 0};
    adBms6830ParseCell(1, ic, A, rx);
    TEST_ASSERT_EQUAL_INT16(-1, ic[0].cell.c_codes[0]);
    TEST_ASSERT_EQUAL_INT16(-32768, ic[0].cell.c_codes[1]);
}

/* Daisy chain: IC 0's 8 bytes come first, then IC 1's. */
void test_parse_cell_two_ics_land_in_the_right_struct(void) {
    uint8_t rx[16] = {
        0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0, 0, /* IC 0 */
        0x0A, 0x00, 0x0B, 0x00, 0x0C, 0x00, 0, 0, /* IC 1 */
    };
    adBms6830ParseCell(2, ic, A, rx);
    TEST_ASSERT_EQUAL_INT16(1, ic[0].cell.c_codes[0]);
    TEST_ASSERT_EQUAL_INT16(3, ic[0].cell.c_codes[2]);
    TEST_ASSERT_EQUAL_INT16(10, ic[1].cell.c_codes[0]);
    TEST_ASSERT_EQUAL_INT16(12, ic[1].cell.c_codes[2]);
}

/* RDCVALL: one 34-byte block per IC holding all 16 cells. */
void test_parse_cell_all_groups_single_ic(void) {
    uint8_t rx[RDCVALL_SIZE] = {0};
    for (int k = 0; k < 16; k++) {
        rx[2 * k] = (uint8_t)(k + 1); /* cell k+1 = k+1 */
    }
    adBms6830ParseCell(1, ic, ALL_GRP, rx);
    for (int k = 0; k < 16; k++) {
        TEST_ASSERT_EQUAL_INT16(k + 1, ic[0].cell.c_codes[k]);
    }
}

/* ---------- Aux (temperature) parsing ---------- */

void test_parse_aux_group_D_fills_codes_9_to_11(void) {
    uint8_t rx[8] = {0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0, 0};
    adBms6830ParseAux(1, ic, D, rx);
    TEST_ASSERT_EQUAL_INT16(1, ic[0].aux.a_codes[9]);
    TEST_ASSERT_EQUAL_INT16(2, ic[0].aux.a_codes[10]);
    TEST_ASSERT_EQUAL_INT16(3, ic[0].aux.a_codes[11]);
}

/* ---------- Status D: per-cell over/under-voltage flags ---------- */

/* Byte 0 = 0b0000_0110: bit1 = cell 1 OV, bit2 = cell 2 UV. */
void test_parse_status_D_ov_uv_bits(void) {
    uint8_t rx[8] = {0x06, 0x00, 0x00, 0x80, 0x00, 0x00, 0, 0};
    adBms6830ParseStatusD(1, ic, rx);
    TEST_ASSERT_EQUAL_UINT8(1, ic[0].statd.c_ov[0]);
    TEST_ASSERT_EQUAL_UINT8(0, ic[0].statd.c_uv[0]);
    TEST_ASSERT_EQUAL_UINT8(1, ic[0].statd.c_uv[1]);
    TEST_ASSERT_EQUAL_UINT8(0, ic[0].statd.c_ov[1]);
    TEST_ASSERT_EQUAL_UINT8(1, ic[0].statd.c_ov[15]); /* byte 3 bit 7 */
}

/* ---------- Serial ID ---------- */

void test_parse_sid_copies_six_bytes_per_ic(void) {
    uint8_t rx[16] = {1, 2, 3, 4, 5, 6, 0, 0, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0, 0};
    adBms6830ParseSID(2, ic, rx);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(((uint8_t[]){1, 2, 3, 4, 5, 6}), ic[0].sid.sid, 6);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(((uint8_t[]){0xA, 0xB, 0xC, 0xD, 0xE, 0xF}), ic[1].sid.sid, 6);
}

/* ---------- Config A / B: create (pack) and parse (unpack) ---------- */

void test_create_configb_byte_layout(void) {
    ic[0].tx_cfgb.vuv = 0x0ABC;
    ic[0].tx_cfgb.vov = 0x0DEF;
    ic[0].tx_cfgb.dtmen = 1;
    ic[0].tx_cfgb.dtrng = 1;
    ic[0].tx_cfgb.dcto = 0x15;
    ic[0].tx_cfgb.dcc = 0x8001; /* cells 1 and 16 discharging */
    adBms6830CreateConfigb(1, ic);

    uint8_t expected[6] = {
        0xBC,               /* VUV[7:0]                 */
        0xFA,               /* VOV[3:0]<<4 | VUV[11:8]  */
        0xDE,               /* VOV[11:4]                */
        0x80 | 0x40 | 0x15, /* DTMEN | DTRNG | DCTO   */
        0x01,               /* DCC[7:0]                 */
        0x80,               /* DCC[15:8]                */
    };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, ic[0].configb.tx_data, 6);
}

/* Round trip: whatever we pack, parsing the same bytes must give it back.
 * This catches any mismatch between the two halves of the bit layout. */
void test_configb_round_trip(void) {
    ic[0].tx_cfgb.vuv = SetUnderVoltageThreshold(2.5f);
    ic[0].tx_cfgb.vov = SetOverVoltageThreshold(4.25f);
    ic[0].tx_cfgb.dtrng = 1;
    ic[0].tx_cfgb.dcto = 7;
    ic[0].tx_cfgb.dcc = 0x5A5A;
    adBms6830CreateConfigb(1, ic);

    uint8_t rx[8] = {0};
    memcpy(rx, ic[0].configb.tx_data, 6);
    adBms6830ParseConfigb(1, ic, rx);

    TEST_ASSERT_EQUAL_UINT16(ic[0].tx_cfgb.vuv, ic[0].rx_cfgb.vuv);
    TEST_ASSERT_EQUAL_UINT16(ic[0].tx_cfgb.vov, ic[0].rx_cfgb.vov);
    TEST_ASSERT_EQUAL_UINT8(1, ic[0].rx_cfgb.dtrng);
    TEST_ASSERT_EQUAL_UINT8(7, ic[0].rx_cfgb.dcto);
    TEST_ASSERT_EQUAL_HEX16(0x5A5A, ic[0].rx_cfgb.dcc);
}

void test_configa_round_trip(void) {
    ic[0].tx_cfga.refon = 1;
    ic[0].tx_cfga.cth = 5;
    ic[0].tx_cfga.flag_d = 0xA5;
    ic[0].tx_cfga.soakon = 1;
    ic[0].tx_cfga.owa = 6;
    ic[0].tx_cfga.gpo = 0x3FF; /* all 10 GPIOs */
    ic[0].tx_cfga.fc = 3;
    adBms6830CreateConfiga(1, ic);

    uint8_t rx[8] = {0};
    memcpy(rx, ic[0].configa.tx_data, 6);
    adBms6830ParseConfiga(1, ic, rx);

    TEST_ASSERT_EQUAL_UINT8(1, ic[0].rx_cfga.refon);
    TEST_ASSERT_EQUAL_UINT8(5, ic[0].rx_cfga.cth);
    TEST_ASSERT_EQUAL_HEX8(0xA5, ic[0].rx_cfga.flag_d);
    TEST_ASSERT_EQUAL_UINT8(1, ic[0].rx_cfga.soakon);
    TEST_ASSERT_EQUAL_UINT8(6, ic[0].rx_cfga.owa);
    TEST_ASSERT_EQUAL_HEX16(0x3FF, ic[0].rx_cfga.gpo);
    TEST_ASSERT_EQUAL_UINT8(3, ic[0].rx_cfga.fc);
}

/* ---------- PWM ---------- */

/* Two 4-bit duty cycles per byte: low nibble = even channel, high = odd. */
void test_create_pwma_packs_two_channels_per_byte(void) {
    for (int k = 0; k < PWMA; k++) {
        ic[0].PwmA.pwma[k] = (uint8_t)(k & 0x0F);
    }
    adBms6830CreatePwma(1, ic);
    TEST_ASSERT_EQUAL_HEX8(0x10, ic[0].pwma.tx_data[0]); /* ch1=1, ch0=0 */
    TEST_ASSERT_EQUAL_HEX8(0x32, ic[0].pwma.tx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0xBA, ic[0].pwma.tx_data[5]); /* ch11=B, ch10=A */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_overvoltage_threshold_4v25);
    RUN_TEST(test_undervoltage_threshold_2v5);
    RUN_TEST(test_thresholds_fit_in_12_bits);
    RUN_TEST(test_configA_flag_set_and_clear);
    RUN_TEST(test_dcc_bit_for_each_cell);
    RUN_TEST(test_set_pwm_duty_cycle_sets_every_channel_of_every_ic);
    RUN_TEST(test_parse_cell_group_A_is_little_endian);
    RUN_TEST(test_parse_cell_each_group_fills_the_right_cells);
    RUN_TEST(test_parse_cell_group_F_fills_only_cell_16);
    RUN_TEST(test_parse_cell_handles_negative_codes);
    RUN_TEST(test_parse_cell_two_ics_land_in_the_right_struct);
    RUN_TEST(test_parse_cell_all_groups_single_ic);
    RUN_TEST(test_parse_aux_group_D_fills_codes_9_to_11);
    RUN_TEST(test_parse_status_D_ov_uv_bits);
    RUN_TEST(test_parse_sid_copies_six_bytes_per_ic);
    RUN_TEST(test_create_configb_byte_layout);
    RUN_TEST(test_configb_round_trip);
    RUN_TEST(test_configa_round_trip);
    RUN_TEST(test_create_pwma_packs_two_channels_per_byte);
    return UNITY_END();
}
