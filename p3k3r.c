#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "crc.h"
#include "z85.h"

#define ETC_CONFIG "/etc/p3k3r.cfg"

static void usage()
{
  fprintf(stderr, "Usage: p3k3r SWITCH-NAME PORT-NAME [CONFIG]\n\n");
}

static int get_port_type(const char *typeconfig,
                         const char *name, uint32_t *type)
{
  FILE *fp;
  char *cfg_port = NULL;

  fp = fopen(typeconfig, "r");
  if (!fp) {
    fprintf(stderr, "Unable to open config: %s\n", typeconfig);
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
  char enc85[64];
  char p3k3r[8];
  const char *swname;
  const char *portname;
  const char *config;
  const char *stdconfig = ETC_CONFIG;
  int split;
  uint32_t crc, portdesc, type, intfnum, foo, portnum;

  if (argc != 3 && argc != 4) {
    usage();
    return -1;
  }

  swname   = argv[1];
  portname = argv[2];
  if (argc == 3) {
    config   = stdconfig;
  } else {
    config   = argv[3];
  }

  /* Calculate CRC32 for switch name */
  crc = crc32((uint8_t *)swname, strlen(swname));

  /* Parse port spec */
  if (parse_port(config, portname, &type, &intfnum, &foo, &portnum, &split) < 0) {
    return -2;
  }

  /*  0...5 -- type    */
  /*  6..11 -- chassis */
  /* 12..17 -- module  */
  /* 18..28 -- portnum */
  /* 29..31 -- hydra   */

  portdesc = (type & 0x3e) |
             (intfnum & 0x3f)  << 6 |
             (foo & 0x3e)      << 12 |
             (portnum & 0x7ff) << 18;

  if (split != -1) {
    /* Set the split number */
    if (split >= 0x8) {
      fprintf(stderr, "Too big split port number: %d\n", split);
      return -2;
    }
    portdesc |= (split - 1) << 29;
  } else {
    portdesc |= 0x7 << 29;
  }

//  sprintf(p3k3r, "%08X%08X",
//          crc,
//          portdesc);

  memcpy(p3k3r, &crc, sizeof(crc));
  memcpy(p3k3r+sizeof(crc), &portdesc, sizeof(portdesc));

  Z85_encode(p3k3r, enc85, 8);

  printf("%s\n", enc85);

  return 0;
}
