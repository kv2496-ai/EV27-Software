/**
 * @file test_known_issues.c
 * @brief Tests that reproduce driver bugs listed in docs/known_issues.md.
 *
 * These are EXPECTED TO FAIL until the bugs are fixed, so they are not part
 * of `make test`. Run them with `make known-issues`.
 *
 * Both bugs are out-of-bounds memory accesses. In normal C nothing visibly
 * happens: the code just corrupts nearby memory. The test build turns on
 * AddressSanitizer, which stops the program and prints a
 * "heap-buffer-overflow" report pointing at the bad line. Because that
 * report ends the process, each test runs in its own process: the Makefile
 * calls this binary once per test name.
 *
 * When a bug is fixed, its test should pass (print PASS); then move it into
 * test_spi_protocol.c so it guards against the bug coming back.
 */
#include <string.h>

#include "adBms6830CmdList.h"
#include "adbms_main.h"
#include "mcuWrapper_fake.h"
#include "unity.h"

static cell_asic ic[6]; /* TOTAL_IC in adBms_Application.h */
static uint8_t reply[1024];

static void fake_transfer(uint8_t* tx, uint8_t* rx, uint16_t size) { memcpy(rx, reply, size); }

void setUp(void) {
    memset(ic, 0, sizeof(ic));
    mcuWrapper_fakes_reset();
    spiWriteReadBytes_fake.custom_fake = fake_transfer;
}
void tearDown(void) {}

/* Known issue #3a: adBmsReadData() sizes its buffer for ONE IC (34 bytes)
 * when reading all cells with RDCVALL, but spiReadData() writes 34 bytes
 * PER IC into it. With 2 ICs that's 34 bytes past the end of the buffer. */
void test_rdcvall_with_two_ics_stays_in_bounds(void) {
    adBmsReadData(2, ic, RDCVALL, Rdcvall, ALL_GRP);
    TEST_PASS();
}

/* Known issue #3b: spiReadData() stores the total reply size in a uint8_t.
 * RDCSALL with 6 ICs needs 66 * 6 = 396 bytes, which wraps to 140, so the
 * driver asks for (and allocates) far too few bytes. */
void test_rdcsall_with_six_ics_requests_full_reply(void) {
    uint8_t rx[RDCSALL_SIZE * 6], pec[6], cnt[6];
    spiReadData(6, RDCSALL, rx, pec, cnt, RDCSALL_SIZE);
    TEST_ASSERT_EQUAL_UINT16(RDCSALL_SIZE * 6, spiWriteReadBytes_fake.arg2_val);
}

int main(int argc, char** argv) {
    const char* only = (argc > 1) ? argv[1] : "";
    UNITY_BEGIN();
    if (!*only || strcmp(only, "rdcvall") == 0) {
        RUN_TEST(test_rdcvall_with_two_ics_stays_in_bounds);
    }
    if (!*only || strcmp(only, "rdcsall") == 0) {
        RUN_TEST(test_rdcsall_with_six_ics_requests_full_reply);
    }
    return UNITY_END();
}
