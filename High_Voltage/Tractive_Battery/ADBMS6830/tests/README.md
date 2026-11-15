# ADBMS6830 driver unit tests

Host-side tests for the ADBMS6830 BMS driver, written with
[Unity](https://github.com/ThrowTheSwitch/Unity) (assertions) and
[fff](https://github.com/meekrosoft/fff) (fake functions). They run on a
laptop with normal `gcc`/`clang`, with no STM32 or BMS hardware.

## Running

```sh
cd High_Voltage/Tractive_Battery/ADBMS6830/tests
make                 # build + run all suites
make known-issues    # reproduce the bugs in ../docs/known_issues.md (expected to fail)
make clean
```

The first `make` downloads pinned versions of Unity (v2.6.1) and fff (v1.1)
into `vendor/` with `git clone`. You need `make`, a C compiler and `git`.

- **macOS:** `xcode-select --install` gives you all three.
- **Windows:** use WSL (Ubuntu), then `sudo apt install build-essential git`.
  MinGW works too, but it has no AddressSanitizer, so run `make SANITIZE=`.

CI runs the same thing on every PR that touches `ADBMS6830/`
(`.github/workflows/adbms6830-tests.yml`).

## Layout

| Path | What it is |
|---|---|
| `test_pec.c` | `Pec15_Calc()` / `pec10_calc()`: known answers, bit-flip detection |
| `test_parse_create.c` | Byte ⇄ struct conversion, thresholds, bit masks, round trips |
| `test_spi_protocol.c` | SPI frames, daisy-chain order, PEC checking on reads, ADC commands (uses fakes) |
| `test_known_issues.c` | Reproduces open bugs; not part of `make` |
| `fakes/mcuWrapper_fake.[ch]` | fff fakes replacing `mcuWrapper.c` (the only hardware layer) |
| `stubs/` | Empty stand-ins for `main.h`, `stm32g4xx_hal.h`, `can_management.h` |
| `support/test_helpers.h` | `make_ic_reply()`: builds a valid 8-byte chip reply |

The driver files are compiled **unmodified**: `stubs/` comes first on the
include path, so the STM32 headers resolve to the empty stubs, and
`mcuWrapper_fake.c` is linked instead of `mcuWrapper.c`.

All tests build with AddressSanitizer and UndefinedBehaviorSanitizer, so an
out-of-bounds write or similar silent C bug makes the test crash with a report
instead of passing by luck.

## Adding a test

1. Pick the suite that matches the function (or make a new `test_<name>.c`
   and add it to `SUITES` in the Makefile).
2. Write a `void test_<what_it_checks>(void)` function. Follow
   arrange → act → assert:
   ```c
   void test_parse_cell_group_B_fills_cells_4_to_6(void) {
       uint8_t rx[8] = {0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0, 0}; /* arrange */
       adBms6830ParseCell(1, ic, B, rx);                           /* act     */
       TEST_ASSERT_EQUAL_INT16(1, ic[0].cell.c_codes[3]);          /* assert  */
   }
   ```
3. Add `RUN_TEST(test_<what_it_checks>);` to that file's `main()`.
4. `make`.

To check a test is actually useful, break the driver on purpose (e.g. swap two
bytes) and confirm the test fails, then undo the change.

### Using the fakes

```c
adBmsCsLow_fake.call_count           // how many times it was called
spiWriteBytes_fake.arg0_val          // first argument of the last call
fff.call_history[0]                  // which fake ran first (call order)
spiWriteReadBytes_fake.custom_fake = my_fn;  // run my_fn instead
```

fff only saves the *pointer* a fake receives, and the driver frees that
buffer right after. To inspect bytes, use a `custom_fake` that copies them
(see `capture_write()` in `test_spi_protocol.c`).
