#include "log.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <errno.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static void setopts(int s) {
  struct l2cap_options opts = {0};
  int err;
  unsigned int size = sizeof(opts);
  err = getsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, &size);

  if (!err) {
    opts.omtu = opts.imtu = 167;
    log_info("Setting sockopts\n");
    err = setsockopt(s, SOL_L2CAP, L2CAP_OPTIONS, &opts, size);
    if (!err)
      log_info("Set sockopts\n");
    else {
      log_info("Unable to set sockopts. %s (%d)\n", strerror(errno), errno);
    }
  } else {
    log_info("Unable to get sockopts\n");
  }
}

void l2cap_connect(char *bd_addr_raw, uint16_t psm) {
  int s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_L2CAP), status;
  if (s == -1) {
    log_info("L2CAP: Could not create a socket %s:%u. %s (%d)\n", bd_addr_raw,
             psm, strerror(errno), errno);
  } else
    log_info("L2CAP: Created socket %s:%u\n", bd_addr_raw, psm);

  struct sockaddr_l2 addr = {0};
  addr.l2_family = AF_BLUETOOTH;
  addr.l2_psm = htobs(psm);
  // See
  // https://elixir.bootlin.com/linux/latest/source/net/bluetooth/l2cap_sock.c#L238
  addr.l2_bdaddr_type = BDADDR_LE_PUBLIC;

  char bd_addr[30];
  strncpy(bd_addr, bd_addr_raw, 30);

  for (size_t i = 0; i < strnlen(bd_addr, 30); i++) {
    if (bd_addr[i] == '_') {
      bd_addr[i] = ':';
    }
  }

  str2ba(bd_addr, &addr.l2_bdaddr);

  setopts(s);
  status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

  if (status == 0)
    log_info("L2CAP: Connected to %s:%u\n", bd_addr, psm);
  else
    log_info("L2CAP: Could not connect to %s:%u. Error %d. %d (%s)\n", bd_addr,
             psm, status, errno, strerror(errno));

  close(s);
}
