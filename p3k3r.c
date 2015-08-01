#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "crc.h"

static void usage()
{
  fprintf(stderr, "usage: p3k3r SWITCH-NAME PORT-NAME CONFIG\n\n");
}

static int get_port_type(const char *typeconfig,
                         const char *name, uint32_t *type)
{
  FILE *fp;
  char *cfg_port = NULL;

  fp = fopen(typeconfig, "r");
  if (!fp) {
    fprintf(stderr, "Unable to open config\n");
    return -1;
  }

  /* Scan line with WORD and NUMBER */
  while (fscanf(fp, "%m[^ ] %u\n", &cfg_port, type) == 2) {
    /* Match read WORD and requested name */
    if (!strcmp(cfg_port, name)) {
      free(cfg_port);
      fclose(fp);
      return 1;
    }
    if (cfg_port)
      free(cfg_port);
    cfg_port = NULL;
  }

  fclose(fp);

  if (cfg_port)
    free(cfg_port);

  fprintf(stderr, "Unknown port type: [%s]\n", name);
  return -2;
}

static int parse_port(const char *typeconfig, const char *portname,
                      uint32_t *type, uint32_t *intfnum, uint32_t *foo,
                      uint32_t *portnum, int *split)
{
  int n;
  int rc = 1;
  char *tp = NULL;
  char *ifn;

  /* Pass 1: parse format with splitted port */
  n = sscanf(portname, "%m[^/]/%u/%u:%d", &tp, foo, portnum, split);
  if (n != 4) {
    /* Pass 2: parse format without splitted port */
    *split = -1;
    n = sscanf(portname, "%m[^/]/%u/%u", &tp, foo, portnum);
    if (n != 3) {
      fprintf(stderr, "Malformed format: [%s]\n", portname);
      rc = -1;
      goto out;
    }
  }

  /* Check for interface number presence */
  ifn = tp + strlen(tp) - 1;

  /* If no digit at the end, raise error */
  if (!isdigit(*ifn))
    goto malformed_port;

  /* Scan digits from the end */
  while (ifn != tp && isdigit(*ifn)) {
    --ifn;
  }

  /* No digits, raise error */
  if (ifn == tp && !isdigit(ifn[1])) {
malformed_port:
    fprintf(stderr, "Malformed port format [%s]\n", tp);
    rc = -2;
    goto out;
  }

  /* Extract interface number */
  *intfnum = atoi(++ifn);

  /* Get port type from config file */
  *ifn = '\0';
  if (get_port_type(typeconfig, tp, type) < 0) {
    rc = -3;
  }

out:
  /* Free buffer allocated by sscanf */
  if (tp)
    free(tp);

  return rc;
}

int main(int argc, const char *argv[])
{
  const char *swname;
  const char *portname;
  const char *config;
  int split;
  uint32_t crc, type, intfnum, foo, portnum;

  if (argc != 4) {
    usage();
    return -1;
  }

  swname   = argv[1];
  portname = argv[2];
  config   = argv[3];

  /* Calculate CRC16 or CRC32 for switch name */
  crc = crc32((uint8_t *)swname, strlen(swname));

  /* Parse port spec */
  if (parse_port(config, portname, &type, &intfnum, &foo, &portnum, &split) < 0) {
    return -2;
  }

  /* Shrink the port number range to 0-8191 */
  portnum &= 0x1fff;

  /* Do some magic to pack a result */
  if (split != -1) {
    /* 15th bit indicate split mode */
    portnum |= (1 << 15);

    /* Set the split number in range of 0-3 */
    portnum |= (((split-1) & 0x03) << 13);
  }

#if 1
  /* Normal pack                */
  /*  8 -- crc                  */
  /*  1 -- type    (actual 3/8) */
  /*  1 -- intfnum (actual 3/8) */
  /*  1 -- foo     (actual 3/8) */
  /*  4 -- split & portnum      */
  /* 15 == total                */
#else
  /* Extensive pack             */
  /*   7 -- crc                 */
  /* 3/8 -- type                */
  /* 3/8 -- intfnum             */
  /* 1/4 -- foo                 */
  /*   2 -- split & portnum     */
  /*  10 == total               */
#endif

  printf("%08X%c%c%c%04X\n",
         crc,
         '0'+(type & 0x3f),
         '0'+(intfnum & 0x3f),
         '0'+(foo & 0x3f),
          portnum);

  return 0;
}
