# Known issues {#known_issues}

Found while documenting the driver (November 2026). None of these have been
fixed yet. Issue 2 is pinned down by
`test_write_pwm_uses_PwmA_not_raw_tx_data` and issue 3 by
`tests/test_known_issues.c`. Issues 1 and 4 are in the application layer,
which the host tests don't build yet.

## 1. Cell balancing writes past the end of `IC[]`

**Where:** `Process_Cell_Balancing()`, adBms_Application.c

It uses full-pack indexing (`ic_master = seg * 2`, `ic_slave = seg * 2 + 1`)
but `TOTAL_IC` is 6, so for segments 3–5 it writes `IC[6]` … `IC[11]`, past
the end of the array. In C that silently corrupts whatever memory comes next.
This only runs when `can_data->balancing_enable` is set.
`Process_Board_Data()` already uses test-bench indexing (`ic_master = seg`),
so the two functions disagree.

Related: `check_balance` is overwritten on every loop pass, so
`TotalPack.balancing_done` only reflects the **last** segment.

## 2. Balancing PWM values set in `adbms_main_init()` are never sent

`adbms_main_init()` copies `0x44` bytes into `IC[n].pwma.tx_data` /
`pwmb.tx_data`, but `adBmsWriteData(..., Pwm, A/B)` calls
`adBms6830CreatePwma/b()` first, which **rebuilds** `tx_data` from
`IC[n].PwmA.pwma[]` / `PwmB.pwmb[]`. Those are still 0, so 0 is what gets
written. Also, `0x4` is `PWM_26_4_PCT`, not 50% as the comment says. Fix:
set `PwmA.pwma[]` (or call `SetPwmDutyCycle()`) instead.

## 3. "Read all" commands overflow their buffer with more than one IC

*Reproduced by `make known-issues` in `tests/` (AddressSanitizer reports a
heap-buffer-overflow in `spiReadData()`).*

**Where:** `adBmsReadData()` with `group == ALL_GRP`, adBms6830GenericType.c

The buffer is sized `RDCVALL_SIZE` (34 bytes, i.e. one IC), but
`spiReadData()` writes `34 * tIC` bytes into it. In addition, `spiReadData()`
stores the total size in a `uint8_t`, which wraps for sizes over 255 (for
example `RDCSALL_SIZE` 66 × 6 ICs = 396). The EV27 main loop only uses the
per-group reads (`RDCVA`…`RDCVF`), so this does not affect the car today,
but do not use the `*_read_rd*all_*` functions with `TOTAL_IC > 1`.

If an unsupported `type` is passed with `ALL_GRP`, `rBuff_size` is never
set (undefined behaviour).

## 4. Duplicate board ID

`ID7_BOTTOM` has the same value as `ID1_BOTTOM` (`0x0001111111`), so
`lookupID()` can never return 14; that board is reported as board 8.

## 5. `CalcPEC10()` is broken and unused

It reads `pDataBuf[nLength]` on every iteration instead of stepping through
the buffer. Nothing calls it (the driver uses `pec10_calc()`), so it should
be deleted to avoid someone using it by mistake.

## 6. Minor

- `mcuWrapper.h` declares `hspi5`, `huart4`, `huart5`, but `mcuWrapper.c`
  uses `hspi1` and `huart2`.
- `adBms6830CmdList.h` defines its arrays in the header, so it can only be
  included from one `.c` file.
- Allocation failures call `exit(0)`. On a microcontroller there is nothing
  to exit to, and the code would be safer using fixed-size buffers.
