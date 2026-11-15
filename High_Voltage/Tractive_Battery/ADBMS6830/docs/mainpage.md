# EV27 ADBMS6830 BMS Driver {#mainpage}

Firmware that reads every cell voltage and temperature in the EV27 tractive
battery using Analog Devices **ADBMS6830** cell-monitor chips, checks them
against safety limits, balances cells, and estimates state of charge (SOC).
It runs on an STM32G474 (`High_Voltage/Tractive_Battery`).

## The four layers

Each layer only calls the one directly below it.

| Layer | Files | What it does | Touches hardware? |
|---|---|---|---|
| **Application** | `program/src/adBms_Application.c` | Pack logic: init, measurement loop, fault flags, balancing, SOC | No (calls the driver) |
| **Protocol** | `lib/src/adBms6830GenericType.c` | Builds/sends SPI frames, computes and checks PEC (CRC) | No (calls the wrapper) |
| **Parse / create** | `lib/src/adBms6830ParseCreate.c` | Converts raw bytes ⇄ named struct fields | No (pure memory) |
| **MCU wrapper** | `program/src/mcuWrapper.c` | `HAL_SPI_*`, chip-select pin, delays | **Yes, the only layer that does** |

Shared types live in `lib/inc/adBms6830Data.h` (the `cell_asic` struct) and
command codes in `lib/inc/adBms6830CmdList.h`.

## How one cell-voltage read works

```
measurement_loop()                               [Application]
 └─ adBmsReadData(TOTAL_IC, IC, RDCVA, Cell, A)  [Protocol]
     ├─ spiReadData()
     │   ├─ Pec15_Calc(cmd)          → 2-byte command PEC
     │   ├─ adBmsWakeupIc()          → toggle CS to wake the chain   [Wrapper]
     │   ├─ adBmsCsLow()                                              [Wrapper]
     │   ├─ spiWriteReadBytes()      → send 4 bytes, receive 8×N      [Wrapper]
     │   ├─ adBmsCsHigh()                                             [Wrapper]
     │   └─ pec10_calc() per IC      → compare to received PEC
     ├─ adBms6830ParseCell()         → IC[n].cell.c_codes[0..2]       [Parse]
     └─ IC[n].cccrc.cell_pec = 0 (good) or 1 (corrupt)
```

Voltage in volts = `c_codes[k] * 0.000150 + 1.5`.

## Using the driver

```c
cell_asic IC[TOTAL_IC];                 // one entry per chip

// Configure
IC[0].tx_cfgb.vov = SetOverVoltageThreshold(4.25f);
adBmsWakeupIc(TOTAL_IC);
adBmsWriteData(TOTAL_IC, IC, WRCFGB, Config, B);

// Measure
adBms6830_Adcv(RD_ON, CONTINUOUS, DCP_OFF, RSTF_OFF, OW_OFF_ALL_CH);
Delay_ms(8);
adBms6830_Snap();
adBmsReadData(TOTAL_IC, IC, RDCVA, Cell, A);   // cells 1–3 of every chip
adBms6830_Unsnap();

if (IC[0].cccrc.cell_pec == 0) {
    float v = IC[0].cell.c_codes[0] * 0.000150f + 1.5f;
}
```

## Current pack configuration

- `TOTAL_IC` = 6 chips, `CELLS_PER_MOD` = 10 cells per chip, so **60 cells**
  and 10 temperature inputs per chip (`TEMP_PER_BOARD`).
- The full-pack design is 6 segments × (14-cell master + 10-cell slave) =
  **144 cells on 12 chips**. Look for the `TODO` comments in
  `adBms_Application.c` that switch between the two.

## Generating these docs

```sh
cd High_Voltage/Tractive_Battery/ADBMS6830
doxygen Doxyfile
# open docs/html/index.html
```

Install Doxygen from doxygen.nl (or `brew install doxygen` /
`sudo apt install doxygen graphviz`). Graphviz is only needed for the call
graphs.

## Testing without hardware

Everything above the MCU wrapper can run on a laptop. Replace the functions
in `mcuWrapper.h` with fakes (the fff library), then use Unity to check the
bytes the driver sends and how it decodes canned replies. The pure functions
(`Pec15_Calc`, `pec10_calc`, `SetOverVoltageThreshold`, all
`adBms6830Parse*` / `adBms6830Create*`) need no fakes at all.

See @ref known_issues for bugs found while writing these docs.
