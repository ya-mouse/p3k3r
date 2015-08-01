#include <stdint.h>

#include "crc.h"

static uint32_t crc32_table[256];

/*
 * Build auxiliary table for parallel byte-at-a-time CRC-32.
 */
#define CRC32_POLY 0x04c11db7     /* AUTODIN II, Ethernet, & FDDI */

static void init_crc32(void)
{
  int i, j;
  uint32_t c;

  for (i = 0; i < 256; ++i) {
    for (c = i << 24, j = 8; j > 0; --j)
      c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
    crc32_table[i] = c;
  }
}

uint32_t crc32(const uint8_t *buf, int len)
{
  const uint8_t *p;
  uint32_t crc;

  if (!crc32_table[1])    /* if not already done, */
    init_crc32();   /* build table */

  crc = 0xffffffff;       /* preload shift register, per CRC-32 spec */
  for (p = buf; len > 0; ++p, --len)
    crc = (crc << 8) ^ crc32_table[(crc >> 24) ^ *p];
  return ~crc;            /* transmit complement, per CRC-32 spec */
}
