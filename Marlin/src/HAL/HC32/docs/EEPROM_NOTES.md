# HC32F460 EEPROM notes

Reference notes for whoever next tries to get persistent settings working on
the Kobra Neo without an SD card. The current build uses
`SDCARD_EEPROM_EMULATION` because flash-emulated EEPROM didn't work in
practice on this board — but the *mechanism* below is what HDSC's own SDK
and the OpenHC32Boot bootloader use, so it's the right starting point.

## Chip variant on TriGorilla Gen V3.0.7 boards

At least one Kobra Neo unit in the wild has been observed with an
**HC32F460KCTA** (256 KiB flash, line C in the part code), not the
**HC32F460KETA** (512 KiB, line E) that the pins file header and the
`HC32F460E_anycubic_kobra_neo` env name imply. The current build env extends
`HC32F460E_base` (`board_upload.maximum_size = 524288`) and links a ~272 KiB
firmware — too large to fit on a 256 KiB chip if every byte is required.

How a 272 KiB image runs on a 256 KiB chip is still unverified. Two
plausible explanations:

- Anycubic's stock bootloader silently writes only what fits (truncates the
  image at ~224 KiB after the 32 KiB bootloader), and your hot-path code
  happens to be linked into the surviving prefix.
- The bootloader copies the image from SD into flash on every boot, so any
  flash sector below the firmware end is rewritten each power-on.

Either way, this has direct consequences for flash-emulated EEPROM (see
below). Before touching the firmware layout, verify the actual chip variant
on **your** board by reading the silkscreen on the LQFP-64 IC:

```
HDSC
HC32F460
K_TA          ← C = 256 KiB, E = 512 KiB
<lot/date>
<lot code>
```

## EFM (Embedded Flash Memory) API — correct init sequence

The HC32F460 DDL ships an EFM driver with `EFM_Unlock`, `EFM_FlashCmd`,
`EFM_SectorErase`, `EFM_SingleProgram`, `EFM_SingleProgramRB`, and
`EFM_SequenceProgram`. Three non-obvious things to get right:

1. **You must call `EFM_FlashCmd(Enable)` after `EFM_Unlock`.** This
   clears `FSTP` (flash-stopped) so the next operation's RDY bit can
   eventually assert. Skipping this step is what made our first attempt
   hang inside `EFM_SectorErase`'s internal `while(RDY != 1)` loop —
   that loop has no break condition, so a never-asserting RDY = forever.

2. **Wait for `EFM_FLAG_RDY = Set` before issuing the first erase/program.**
   Bound the wait yourself; the DDL's internal RDY loops are unbounded.

3. **Don't disable interrupts during the operation.** OpenHC32Boot
   deliberately doesn't, even though doing so seems intuitive. The chip
   stalls instruction prefetch internally during erase, so ISRs pause
   instead of executing against erase-mode flash. Marlin's stepper/temp
   ISRs are fine with a brief stall (~18 ms for an 8 KiB erase, ~16 ms
   for a 4 KiB sequence program). Disabling IRQs blocks the watchdog and
   trades a brief hiccup for a chance of permanent hang.

Reference (HDSC SDK `efm_seqence_program/source/main.c` and
[OpenHC32Boot/src/modules/flash.cpp](https://github.com/shadow578/OpenHC32Boot/blob/main/src/modules/flash.cpp)):

```c
EFM_Unlock();
EFM_FlashCmd(Enable);                                 // FSTP = 0
while (EFM_GetFlagStatus(EFM_FLAG_RDY) != Set) { }    // (add a bounded timeout)

EFM_SectorErase(sector_base_addr);                    // 8 KiB sector
for (size_t i = 0; i < words; ++i)                    // word-by-word with read-back
    EFM_SingleProgramRB(addr + i * 4, src[i]);

EFM_FlashCmd(Disable);                                // FSTP = 1 (idle)
EFM_Lock();
```

OpenHC32Boot prefers `EFM_SingleProgramRB` over `EFM_SequenceProgram` so
each word is read back and any program error fails fast. That's the better
pattern for an EEPROM commit.

### Flash geometry

- **Sector size: 8 KiB** (8192 bytes). Confirmed both by the HDSC SDK
  example (`FLASH_SECTOR62 = 0x7C000`, `FLASH_SECTOR61 = 0x7A000`, Δ =
  0x2000) and by OpenHC32Boot's `erase_sector_size = 8192`.
- **Write granularity: 32-bit words**, word-aligned.
- **Endurance: ~10,000 erase cycles per sector** per the datasheet — fine
  for normal M500 use, but a feature that auto-saves (e.g., aggressive
  power-loss recovery) could chew through that faster.
- DDL build flag to enable the driver: `board_build.ddl.efm = true` in
  `ini/hc32.ini`.

## Why our flash-EEPROM attempt didn't work

We reserved the top 8 KiB sector (0x7E000-0x7FFFF on the 512 KiB layout
the env assumes) by capping `board_upload.maximum_size`. With the correct
init sequence above the EFM commit *should* succeed on a real KETA. On
the unit we tested:

- The hang at first M500 is consistent with one of two failure modes:
  - Chip is genuinely KCTA (256 KiB) → `0x7E000` is outside physical
    flash → write to that address never asserts RDY → the DDL's unbounded
    busy-wait spins forever.
  - Bootloader has ICG-set write protection covering the top of flash.

A workable strategy for someone who picks this up again:

1. **Confirm chip variant** first (silkscreen, or write a tiny test build
   that probes flash size via a known-empty-then-erase sequence).
2. **If KCTA (256 KiB):** target the EEPROM sector at the top of *that*
   chip (`0x3E000`), and trim Marlin to fit firmware in `0x36000` bytes
   (216 KiB code area after bootloader and reserved EEPROM). Switching
   `TFT_COLOR_UI` to `TFT_CLASSIC_UI` reclaims ~44 KiB and is the biggest
   single saving.
3. **Verify the bootloader doesn't rewrite the EEPROM region** on every
   boot. If it does, flash EEPROM is structurally impossible without
   replacing the bootloader (e.g., with OpenHC32Boot — see the
   `HC32F460C_openhc32boot` env in `ini/hc32.ini`).
4. **If even one of the above fails:** fall back to either an external
   I²C EEPROM (the HC32 HAL already has `eeprom_bl24cxx.cpp` and
   `eeprom_if_iic.cpp`; just wire SDA/SCL to free GPIOs and add
   `IIC_BL24CXX_EEPROM` + pin defines to the pins file) or to keeping
   SDCARD emulation but ensuring Marlin's compiled defaults are
   sufficient for the printer to home cleanly when the card is missing.
