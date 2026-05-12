/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2024 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

//
// Anycubic Kobra Neo — Trigorilla Pro F1 V1 (HC32F460KETA, 512K Flash)
// Stock bootloader: 0x00000000–0x000087FF, application: 0x00008800
// Final firmware.bin must be padded with 0xFF to 0x77800 bytes for the
// stock bootloader to accept it from SD card.
//
#include "env_validate.h"

#if HAS_MULTI_HOTEND || E_STEPPERS > 1
  #error "ANYCUBIC_KOBRA_NEO supports only one hotend / E-stepper."
#endif

#ifndef BOARD_INFO_NAME
  #define BOARD_INFO_NAME "Anycubic Kobra Neo (Trigorilla Pro F1 V1)"
#endif
#ifndef DEFAULT_MACHINE_NAME
  #define DEFAULT_MACHINE_NAME "Kobra Neo"
#endif

// Display is SPI TFT (ST7789V on SPI2), not serial DGUS.
// Do NOT define BOARD_LCD_SERIAL_PORT — it would initialize USART4 on
// PB10/PH2 which conflicts with BTN_EN1 (encoder) and X_STOP_PIN (endstop).

//
// Release JTAG pins for GPIO use:
//   PA15 (JTDI) = stepper enable (shared)
//   PB3  (JTDO) = encoder BTN_EN2
//   PB4  (NJTRST) = encoder BTN_ENC
//
#define DISABLE_JTAG

//
// Force heater/fan pins to GPIO output mode early, matching the fork's
// BSP led_pin_init(). Without this, these pins may remain in timer
// alternate-function mode and GPIO writes have no effect.
//
#define BOARD_PREINIT() do { \
  OUT_WRITE(PB6, HIGH); /* MOSFET power enable — required for heaters/fans */ \
  OUT_WRITE(PB5, LOW);  /* FAN0 - part cooling */ \
  OUT_WRITE(PB8, LOW);  /* HEATER_0 */ \
  OUT_WRITE(PB9, LOW);  /* HEATER_BED */ \
  OUT_WRITE(PB13, LOW); /* FAN1 - E0 auto-fan */ \
} while(0)

//
// 8 MHz onboard crystal
//
#ifndef BOARD_XTAL_FREQUENCY
  #define BOARD_XTAL_FREQUENCY           8000000
#endif

//
// EEPROM — the upstream HC32 HAL has no flash-emulation backend, so we
// emulate EEPROM on the SD card (which is always present on this board).
//
#if NO_EEPROM_SELECTED
  #define SDCARD_EEPROM_EMULATION
  #define MARLIN_EEPROM_SIZE           0x1000U   // 4 KB on-disk
  #undef NO_EEPROM_SELECTED
#endif

//
// Limit switches
//
#define X_STOP_PIN                          PH2
#define Y_STOP_PIN                          PC13
#define Z_STOP_PIN                          PC14

// Anycubic "LeviQ" auto-level probe data line. The sensor in the nozzle
// assembly (Hall / magnetic-field based) has its own MCU that reports
// trigger status to the mainboard over UART RX on PA1 — it's not a
// level-triggered digital input. Custom probe glue (see HAL/feature) is
// needed to listen on this line. Defined as Z_MIN_PROBE_PIN so Marlin's
// probe state machine can reference it.
#ifndef Z_MIN_PROBE_PIN
  #define Z_MIN_PROBE_PIN                   PA1
#endif

//
// Filament runout
//
#ifndef FIL_RUNOUT_PIN
  #define FIL_RUNOUT_PIN                    PC15
#endif

//
// Steppers — single shared enable line on PA15.
//
#define X_ENABLE_PIN                        PA15
#define X_STEP_PIN                          PA12
#define X_DIR_PIN                           PA11

#define Y_ENABLE_PIN                X_ENABLE_PIN
#define Y_STEP_PIN                          PA9
#define Y_DIR_PIN                           PA8

#define Z_ENABLE_PIN                X_ENABLE_PIN
#define Z_STEP_PIN                          PC7
#define Z_DIR_PIN                           PC6

#define E0_ENABLE_PIN               X_ENABLE_PIN
#define E0_STEP_PIN                         PB15
#define E0_DIR_PIN                          PB14

//
// TMC2208 single-wire UART — software serial only (TX == RX on PB2),
// shared across X/Y/Z/E0. The stock Anycubic boards leave CHOPCONF.toff
// at 0 on power-up (drivers' MOSFET stage disabled), so every boot must
// write CHOPCONF over UART to bring the drivers online. TMC6609 selects
// the Anycubic-specific init sequence (i_scale_analog=1, diss2vs=1,
// matched against the working stock-fork firmware) in trinamic.cpp.
//
#define TMC6609
#if HAS_TMC_UART
  #define X_SERIAL_TX_PIN                   PB2
  #define X_SERIAL_RX_PIN                   PB2
  #define Y_SERIAL_TX_PIN          X_SERIAL_TX_PIN
  #define Y_SERIAL_RX_PIN          X_SERIAL_RX_PIN
  #define Z_SERIAL_TX_PIN          X_SERIAL_TX_PIN
  #define Z_SERIAL_RX_PIN          X_SERIAL_RX_PIN
  #define E0_SERIAL_TX_PIN         X_SERIAL_TX_PIN
  #define E0_SERIAL_RX_PIN         X_SERIAL_RX_PIN
  #ifndef TMC_BAUD_RATE
    #define TMC_BAUD_RATE                   19200
  #endif
#endif

//
// Temperature sensors
//
#define TEMP_0_PIN                          PC3   // Hotend  (ADC1)
#define TEMP_BED_PIN                        PC1   // Bed     (ADC1)

//
// Heaters
//
#define HEATER_0_PIN                        PB8
#define HEATER_BED_PIN                      PB9

//
// Fans
//   FAN0 = part-cooling (model), FAN1 = E0 auto-cool, FAN2 = controller box
//
#define FAN0_PIN                            PB5     // part-cooling
#define FAN1_PIN                            PB13    // E0 hotend cooling (auto-fan)
#define FAN_SOFT_PWM
// E0 auto-fan: let Marlin auto-derive E0_AUTO_FAN_PIN from FAN1_PIN
// when EXTRUDER_AUTO_FAN_TEMPERATURE is enabled in Configuration_adv.h.

//
// Misc I/O
//
#define BEEPER_PIN                          PB7
#define LED_PIN                             -1
#define CASE_LIGHT_PIN                      -1

// Power loss detection / monitor (PSU sense divider on PC2)
#define POWER_LOSS_PIN                      PC2
#define POWER_MONITOR_VOLTAGE_PIN           PC2

//
// SD card — onboard SDIO, only visible to the printer (no host pass-through).
//
#define SD_DETECT_PIN                       PA10
#define SDCARD_CONNECTION ONBOARD
#define ONBOARD_SDIO
#define NO_SD_HOST_DRIVE

#define BOARD_SDIO_D0                       PC8
#define BOARD_SDIO_D1                       PC9
#define BOARD_SDIO_D2                       PC10
#define BOARD_SDIO_D3                       PC11
#define BOARD_SDIO_CLK                      PC12
#define BOARD_SDIO_CMD                      PD2
#define BOARD_SDIO_DET               SD_DETECT_PIN

//
// USART pin mapping
//   USART2 = host / debug header
//   USART4 = LCD (Anycubic serial display protocol)
//
// USART2 = host / debug header
#define BOARD_USART2_TX_PIN                 PA2
#define BOARD_USART2_RX_PIN                 PA3

// USART4 NOT used — PB10 is BTN_EN1 (encoder), PH2 is X_STOP_PIN (endstop).

//
// TFT SPI display (ST7796, directly wired on SPI2)
//
// Pin assignments from the Anycubic Kobra Neo schematic / fork firmware.
// The display is directly connected to SPI2 with hardware CS (NSS0).
// DC/RS is directly controlled as GPIO.
//
#if HAS_SPI_TFT
  #define TFT_CS_PIN                        PB1   // SPI2_NSS0
  #define TFT_A0_PIN                        PB0   // DC / RS (command / data select)
  #define TFT_SCK_PIN                       PC5   // SPI2_SCK
  #define TFT_MOSI_PIN                      PC4   // SPI2_MOSI
  // No MISO -- display is write-only on this board.

  // PB4 is the encoder push button on the Kobra Neo. The fork defines it
  // as TFT_RST but never actually pulses it (the reset code in
  // bsp_spi_tft.cpp is completely commented out). Upstream Marlin's
  // TFT_IO::initTFT() will pulse TFT_RESET_PIN if PIN_EXISTS(TFT_RESET),
  // which would glitch the encoder input. So we do NOT define TFT_RESET_PIN
  // here -- the ST7796 powers up in a known state via its internal POR.
  //#define TFT_RESET_PIN                   PB4

  #define TFT_BACKLIGHT_PIN                 PC0
#endif

//
// Rotary encoder knob (directly wired, active LOW)
// PB10 is also USART4_TX, but when using SPI TFT it is free for the encoder.
// PB4 is shared with the (unused) TFT reset -- see note above.
//
#if HAS_WIRED_LCD
  #define BTN_EN1                           PB10
  #define BTN_EN2                           PB3
  #define BTN_ENC                           PB4
#endif
