/**
 * @file test_spi_protocol.c
 * @brief Tests for adBms6830GenericType.c: building SPI frames, writing to
 *        and reading from the daisy chain, and the ADC start commands.
 *
 * These functions call the hardware layer (mcuWrapper.h). In this build those
 * calls go to fff fakes, so the tests can:
 *   - capture every byte the driver "sends", and
 *   - choose the bytes the driver "receives" (good replies, corrupt replies).
 */
#include <string.h>

#include "adBms6830CmdList.h" /* WRCFGB, RDCVA, ... (only include from one .c) */
#include "adbms_main.h"
#include "mcuWrapper_fake.h"
#include "test_helpers.h"
#include "unity.h"

static cell_asic ic[2];

/* ---------- Custom fakes ----------
 * fff stores only the *pointer* a fake was given. The driver frees or reuses
 * that buffer right after the call, so to inspect the bytes later these
 * custom fakes copy them into static arrays. */

static uint8_t sent[256]; /* bytes passed to spiWriteBytes()            */
static uint16_t sent_len;
static uint8_t sent_cmd[4];    /* command frame passed to spiWriteReadBytes() */
static uint8_t reply[256];     /* bytes spiWriteReadBytes() will "receive"    */
static uint16_t requested_len; /* how many bytes the driver asked to receive  */

static void capture_write(uint16_t size, uint8_t* tx) {
    memcpy(sent, tx, size);
    sent_len = size;
}

static void fake_transfer(uint8_t* tx, uint8_t* rx, uint16_t size) {
    memcpy(sent_cmd, tx, 4);
    memcpy(rx, reply, size);
    requested_len = size;
}

void setUp(void) {
    memset(ic, 0, sizeof(ic));
    memset(sent, 0, sizeof(sent));
    memset(reply, 0, sizeof(reply));
    sent_len = 0;
    requested_len = 0;
    mcuWrapper_fakes_reset();
    spiWriteBytes_fake.custom_fake = capture_write;
    spiWriteReadBytes_fake.custom_fake = fake_transfer;
}
void tearDown(void) {}

/* ---------- spiSendCmd: 2 command bytes + PEC15, inside chip select ---------- */

void test_send_cmd_frame_is_command_plus_pec(void) {
    uint8_t cmd[2] = {0x00, 0x04};
    spiSendCmd(cmd);
    uint8_t expected[4] = {0x00, 0x04, 0x07, 0xC2};
    TEST_ASSERT_EQUAL_UINT16(4, sent_len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, sent, 4);
}

void test_send_cmd_wraps_transfer_in_chip_select(void) {
    uint8_t cmd[2] = {0x00, 0x04};
    spiSendCmd(cmd);
    TEST_ASSERT_EQUAL_UINT(3, fff.call_history_idx);
    TEST_ASSERT_EQUAL_PTR((void*)adBmsCsLow, fff.call_history[0]);
    TEST_ASSERT_EQUAL_PTR((void*)spiWriteBytes, fff.call_history[1]);
    TEST_ASSERT_EQUAL_PTR((void*)adBmsCsHigh, fff.call_history[2]);
}

/* ---------- spiWriteData: per-IC data, last IC first ---------- */

void test_write_data_frame_length(void) {
    uint8_t data[12] = {0};
    spiWriteData(2, WRCFGB, data);
    TEST_ASSERT_EQUAL_UINT16(4 + 8 * 2, sent_len);
}

/* The first chunk shifted into the chain ends up in the FARTHEST IC, so the
 * driver must send IC 1's data before IC 0's. */
void test_write_data_sends_last_ic_first(void) {
    uint8_t data[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,  /* IC 0 */
                        0x10, 0x11, 0x12, 0x13, 0x14, 0x15}; /* IC 1 */
    spiWriteData(2, WRCFGB, data);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(&data[6], &sent[4], 6);  /* IC 1 first  */
    TEST_ASSERT_EQUAL_HEX8_ARRAY(&data[0], &sent[12], 6); /* IC 0 second */
}

void test_write_data_appends_valid_pec10_to_each_chunk(void) {
    uint8_t data[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    spiWriteData(2, WRCFGB, data);
    for (int chunk = 0; chunk < 2; chunk++) {
        uint8_t* p = &sent[4 + chunk * 8];
        uint16_t got = (uint16_t)((p[6] << 8) | p[7]);
        TEST_ASSERT_EQUAL_HEX16(pec10_calc(false, 6, p), got);
    }
}

void test_write_data_command_has_pec15(void) {
    uint8_t data[6] = {0};
    spiWriteData(1, WRCFGB, data);
    uint16_t pec = Pec15_Calc(2, WRCFGB);
    TEST_ASSERT_EQUAL_HEX8(WRCFGB[0], sent[0]);
    TEST_ASSERT_EQUAL_HEX8(WRCFGB[1], sent[1]);
    TEST_ASSERT_EQUAL_HEX8(pec >> 8, sent[2]);
    TEST_ASSERT_EQUAL_HEX8(pec & 0xFF, sent[3]);
}

/* ---------- spiReadData: PEC checking on what comes back ---------- */

void test_read_data_accepts_valid_reply_and_extracts_counter(void) {
    uint8_t d0[6] = {0xBC, 0x34, 0x10, 0x27, 0x00, 0x00};
    uint8_t d1[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    make_ic_reply(&reply[0], d0, 5);
    make_ic_reply(&reply[8], d1, 42);

    uint8_t rx[16], pec_err[2] = {9, 9}, cnt[2];
    spiReadData(2, RDCVA, rx, pec_err, cnt, RX_DATA);

    TEST_ASSERT_EQUAL_UINT8(0, pec_err[0]);
    TEST_ASSERT_EQUAL_UINT8(0, pec_err[1]);
    TEST_ASSERT_EQUAL_UINT8(5, cnt[0]);
    TEST_ASSERT_EQUAL_UINT8(42, cnt[1]);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(reply, rx, 16);
}

void test_read_data_requests_8_bytes_per_ic(void) {
    uint8_t rx[16], pec_err[2], cnt[2];
    spiReadData(2, RDCVA, rx, pec_err, cnt, RX_DATA);
    TEST_ASSERT_EQUAL_UINT16(16, requested_len);
    TEST_ASSERT_EQUAL_UINT8(2, adBmsWakeupIc_fake.arg0_val);
}

/* A dead or disconnected bus returns all 0x00 (or 0xFF). Neither is a valid
 * frame, so every IC must be flagged as a PEC error. */
void test_read_data_flags_all_zero_reply(void) {
    uint8_t rx[16], pec_err[2], cnt[2];
    spiReadData(2, RDCVA, rx, pec_err, cnt, RX_DATA);
    TEST_ASSERT_EQUAL_UINT8(1, pec_err[0]);
    TEST_ASSERT_EQUAL_UINT8(1, pec_err[1]);
}

void test_read_data_flags_all_ff_reply(void) {
    memset(reply, 0xFF, 16);
    uint8_t rx[16], pec_err[2], cnt[2];
    spiReadData(2, RDCVA, rx, pec_err, cnt, RX_DATA);
    TEST_ASSERT_EQUAL_UINT8(1, pec_err[0]);
    TEST_ASSERT_EQUAL_UINT8(1, pec_err[1]);
}

/* Noise hits one IC's data: only that IC is flagged. */
void test_read_data_flags_only_the_corrupted_ic(void) {
    uint8_t d[6] = {1, 2, 3, 4, 5, 6};
    make_ic_reply(&reply[0], d, 1);
    make_ic_reply(&reply[8], d, 1);
    reply[8 + 2] ^= 0x10; /* flip one bit in IC 1's data */

    uint8_t rx[16], pec_err[2], cnt[2];
    spiReadData(2, RDCVA, rx, pec_err, cnt, RX_DATA);
    TEST_ASSERT_EQUAL_UINT8(0, pec_err[0]);
    TEST_ASSERT_EQUAL_UINT8(1, pec_err[1]);
}

/* ---------- adBmsReadData: full read path into the cell_asic struct ---------- */

void test_read_cell_group_A_end_to_end(void) {
    uint8_t d0[6] = {0xBC, 0x34, 0x00, 0x00, 0x00, 0x00}; /* cell 1 = 13500 */
    uint8_t d1[6] = {0x10, 0x27, 0x00, 0x00, 0x00, 0x00}; /* cell 1 = 10000 */
    make_ic_reply(&reply[0], d0, 3);
    make_ic_reply(&reply[8], d1, 3);

    adBmsReadData(2, ic, RDCVA, Cell, A);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(RDCVA, sent_cmd, 2);
    TEST_ASSERT_EQUAL_INT16(13500, ic[0].cell.c_codes[0]);
    TEST_ASSERT_EQUAL_INT16(10000, ic[1].cell.c_codes[0]);
    TEST_ASSERT_EQUAL_UINT8(0, ic[0].cccrc.cell_pec);
    TEST_ASSERT_EQUAL_UINT8(0, ic[1].cccrc.cell_pec);
    TEST_ASSERT_EQUAL_UINT8(3, ic[0].cccrc.cmd_cntr);
}

void test_read_cell_corrupt_reply_sets_cell_pec(void) {
    adBmsReadData(2, ic, RDCVA, Cell, A); /* reply is all zeros */
    TEST_ASSERT_EQUAL_UINT8(1, ic[0].cccrc.cell_pec);
    TEST_ASSERT_EQUAL_UINT8(1, ic[1].cccrc.cell_pec);
}

/* ---------- adBmsWriteData: struct fields -> bytes on the wire ---------- */

void test_write_config_b_sends_packed_thresholds(void) {
    ic[0].tx_cfgb.vov = SetOverVoltageThreshold(4.25f); /* 1145 = 0x479 */
    ic[0].tx_cfgb.vuv = SetUnderVoltageThreshold(2.5f); /* 416  = 0x1A0 */
    adBmsWriteData(1, ic, WRCFGB, Config, B);

    TEST_ASSERT_EQUAL_UINT8(1, adBmsWakeupIc_fake.call_count);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(WRCFGB, sent, 2);
    TEST_ASSERT_EQUAL_HEX8(0xA0, sent[4]); /* VUV[7:0]                */
    TEST_ASSERT_EQUAL_HEX8(0x91, sent[5]); /* VOV[3:0]<<4 | VUV[11:8] */
    TEST_ASSERT_EQUAL_HEX8(0x47, sent[6]); /* VOV[11:4]               */
}

/* Documents current behaviour (see docs/known_issues.md #2): writing PWM
 * rebuilds pwma.tx_data from PwmA.pwma[], so anything put directly into
 * tx_data beforehand is thrown away. Set PwmA.pwma[] / SetPwmDutyCycle(). */
void test_write_pwm_uses_PwmA_not_raw_tx_data(void) {
    memset(ic[0].pwma.tx_data, 0x44, TX_DATA); /* what adbms_main_init() does */
    adBmsWriteData(1, ic, WRPWM1, Pwm, A);
    uint8_t zeros[6] = {0};
    TEST_ASSERT_EQUAL_HEX8_ARRAY(zeros, &sent[4], 6);

    SetPwmDutyCycle(1, ic, PWM_26_4_PCT); /* the way that works */
    adBmsWriteData(1, ic, WRPWM1, Pwm, A);
    uint8_t expected[6] = {0x44, 0x44, 0x44, 0x44, 0x44, 0x44};
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, &sent[4], 6);
}

/* ---------- ADC start commands ---------- */

/* cmd[0] = 0x02 + RD, cmd[1] = CONT<<7 | DCP<<4 | RSTF<<2 | OWCS | 0x60 */
void test_adcv_command_bytes(void) {
    adBms6830_Adcv(RD_ON, CONTINUOUS, DCP_OFF, RSTF_OFF, OW_OFF_ALL_CH);
    TEST_ASSERT_EQUAL_HEX8(0x03, sent[0]);
    TEST_ASSERT_EQUAL_HEX8(0xE0, sent[1]);

    adBms6830_Adcv(RD_OFF, SINGLE, DCP_ON, RSTF_ON, OW_ON_ALL_CH);
    TEST_ASSERT_EQUAL_HEX8(0x02, sent[0]);
    TEST_ASSERT_EQUAL_HEX8(0x77, sent[1]);
}

void test_snap_and_unsnap_send_their_commands(void) {
    adBms6830_Snap();
    TEST_ASSERT_EQUAL_HEX8_ARRAY(SNAP, sent, 2);
    adBms6830_Unsnap();
    TEST_ASSERT_EQUAL_HEX8_ARRAY(UNSNAP, sent, 2);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_send_cmd_frame_is_command_plus_pec);
    RUN_TEST(test_send_cmd_wraps_transfer_in_chip_select);
    RUN_TEST(test_write_data_frame_length);
    RUN_TEST(test_write_data_sends_last_ic_first);
    RUN_TEST(test_write_data_appends_valid_pec10_to_each_chunk);
    RUN_TEST(test_write_data_command_has_pec15);
    RUN_TEST(test_read_data_accepts_valid_reply_and_extracts_counter);
    RUN_TEST(test_read_data_requests_8_bytes_per_ic);
    RUN_TEST(test_read_data_flags_all_zero_reply);
    RUN_TEST(test_read_data_flags_all_ff_reply);
    RUN_TEST(test_read_data_flags_only_the_corrupted_ic);
    RUN_TEST(test_read_cell_group_A_end_to_end);
    RUN_TEST(test_read_cell_corrupt_reply_sets_cell_pec);
    RUN_TEST(test_write_config_b_sends_packed_thresholds);
    RUN_TEST(test_write_pwm_uses_PwmA_not_raw_tx_data);
    RUN_TEST(test_adcv_command_bytes);
    RUN_TEST(test_snap_and_unsnap_send_their_commands);
    return UNITY_END();
}
