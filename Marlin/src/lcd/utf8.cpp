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
 * @file    utf8.cpp
 * @brief   Helper functions for UTF-8 strings
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-08-19
 * @copyright GPL/BSD
 */

#include "../inc/MarlinConfig.h"

#if HAS_WIRED_LCD
  #include "ultralcd.h"
  #include "../MarlinCore.h"
#endif

#include "utf8.h"

uint8_t read_byte_ram(const uint8_t *str) { return *str; }
uint8_t read_byte_rom(const uint8_t *str) { return pgm_read_byte(str); }

// Is the passed byte the first byte of a UTF-8 char sequence?
static inline bool utf8_is_start_byte_of_char(const uint8_t b) {
  return 0x80 != (b & 0xC0);
}

/**
 * Get the character at pstart, interpreting UTF8 multibyte sequences.
 * Return the pointer to the next character.
 */
const uint8_t* get_utf8_value_cb(const uint8_t *pstart, read_byte_cb_t cb_read_byte, lchar_t &pval) {
  uint32_t val = 0;
  const uint8_t *p = pstart;

  #define NEXT_6_BITS() do{ val <<= 6; p++; valcur = cb_read_byte(p); val |= (valcur & 0x3F); }while(0)

  uint8_t valcur = cb_read_byte(p);
  if (0 == (0x80 & valcur)) {
    val = valcur;
    p++;
  }
  else if (0xC0 == (0xE0 & valcur)) {
    val = valcur & 0x1F;
    NEXT_6_BITS();
    p++;
  }
  #if MAX_UTF8_CHAR_SIZE >= 3
    else if (0xE0 == (0xF0 & valcur)) {
      val = valcur & 0x0F;
      NEXT_6_BITS();
      NEXT_6_BITS();
      p++;
    }
  #endif
  #if MAX_UTF8_CHAR_SIZE >= 4
    else if (0xF0 == (0xF8 & valcur)) {
      val = valcur & 0x07;
      NEXT_6_BITS();
      NEXT_6_BITS();
      NEXT_6_BITS();
      p++;
    }
  #endif
  #if MAX_UTF8_CHAR_SIZE >= 5
    else if (0xF8 == (0xFC & valcur)) {
      val = valcur & 0x03;
      NEXT_6_BITS();
      NEXT_6_BITS();
      NEXT_6_BITS();
      NEXT_6_BITS();
      p++;
    }
  #endif
  #if MAX_UTF8_CHAR_SIZE >= 6
    else if (0xFC == (0xFE & valcur)) {
      val = valcur & 0x01;
      NEXT_6_BITS();
      NEXT_6_BITS();
      NEXT_6_BITS();
      NEXT_6_BITS();
      NEXT_6_BITS();
      p++;
    }
  #endif
  else if (!utf8_is_start_byte_of_char(valcur))
    for (; !utf8_is_start_byte_of_char(valcur); ) { p++; valcur = cb_read_byte(p); }
  else
    for (; 0xFC < (0xFE & valcur); ) { p++; valcur = cb_read_byte(p); }

  pval = val;

  return p;
}

static inline uint8_t utf8_byte_pos_by_char_num_cb(const char *pstart, read_byte_cb_t cb_read_byte, const uint8_t charnum) {
  uint8_t *p = (uint8_t *)pstart;
  uint8_t char_idx = 0;
  uint8_t byte_idx = 0;
  for (;;) {
    const uint8_t b = cb_read_byte(p + byte_idx);
    if (!b) return byte_idx; // Termination byte of string
    if (utf8_is_start_byte_of_char(b)) {
      char_idx++;
      if (char_idx == charnum + 1) return byte_idx;
    }
    byte_idx++;
  }
}

uint8_t utf8_byte_pos_by_char_num(const char *pstart, const uint8_t charnum) {
  return utf8_byte_pos_by_char_num_cb(pstart, read_byte_ram, charnum);
}

uint8_t utf8_byte_pos_by_char_num_P(PGM_P pstart, const uint8_t charnum) {
  return utf8_byte_pos_by_char_num_cb(pstart, read_byte_rom, charnum);
}
