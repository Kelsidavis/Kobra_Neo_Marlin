/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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

/**
 * lcd/extui/anycubic_kobra_neo/dgus_tft_defs.h
 */

#pragma once

#include "../anycubic/common_defs.h"

// TFT panel commands
#define AC_msg_bed_temp_abnormal        F("J28")

// TFT panel messages
#define MARLIN_msg_probing_point        PSTR("Probing Point ")
#define MARLIN_msg_probe_preheat_start  PSTR("Probe preheat start")
#define MARLIN_msg_probe_preheat_stop   PSTR("Probe preheat stop")
#define MARLIN_msg_media_removed        PSTR("Media Removed")

#define AC_cmnd_manual_unload_filament_first_in PSTR("M83\nG1 E10 F300\nM82")

namespace Anycubic {

  enum paused_state_t : uint8_t {
    AC_paused_heater_timed_out,
    AC_paused_filament_lack,
    AC_paused_purging_filament,
    AC_paused_idle
  };

  enum printer_state_t : uint8_t {
    AC_printer_idle,
    AC_printer_probing,
    AC_printer_printing,
    AC_printer_pausing,
    AC_printer_paused,
    AC_printer_stopping,
    AC_printer_stopping_from_media_remove,
    AC_printer_resuming_from_power_outage
  };

} // Anycubic
