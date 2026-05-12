/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2023 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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

/**
 * HC32F460 SPI TFT driver
 * Uses hardware SPI2 for the ST7796 display on the Anycubic Kobra Neo.
 *
 * Pin mapping (directly from the Anycubic fork):
 *   SPI2_SCK   = PC5
 *   SPI2_MOSI  = PC4
 *   TFT_CS     = PB1  (directly driven by SPI2 NSS0 hardware)
 *   TFT_DC/RS  = PB0  (GPIO, directly controlled for cmd/data)
 *   TFT_RST    = PB4  (optional, typically shared with encoder button)
 *   Backlight  = PC0  (GPIO)
 */

#include "../../../inc/MarlinConfig.h"

#ifndef LCD_READ_ID
  #define LCD_READ_ID  0x04
#endif
#ifndef LCD_READ_ID4
  #define LCD_READ_ID4 0xD3
#endif

#define DATASIZE_8BIT    0
#define DATASIZE_16BIT   1
#define TFT_IO_DRIVER    TFT_SPI
#define DMA_MAX_WORDS    0xFFFF

class TFT_SPI {
private:
  static uint32_t readID(const uint16_t inReg);
  static void transmit(uint16_t data);
  static void transmit(uint32_t memoryIncrease, uint16_t *data, uint16_t count);

public:
  static void init();
  static uint32_t getID();
  static bool isBusy();
  static void abort();

  static void dataTransferBegin(uint16_t dataWidth = DATASIZE_16BIT);
  static void dataTransferEnd();
  static void dataTransferAbort();

  static void writeData(uint16_t data) { transmit(data); }
  static void writeReg(const uint16_t inReg);

  // No DMA on this platform -- blocking transfers only.
  // The _DMA variants just call the blocking versions.
  static void writeSequence_DMA(uint16_t *data, uint16_t count) { writeSequence(data, count); }
  static void writeMultiple_DMA(uint16_t color, uint16_t count) { writeMultiple(color, (uint32_t)count); }

  static void writeSequence(uint16_t *data, uint16_t count);
  static void writeMultiple(uint16_t color, uint32_t count);
};
