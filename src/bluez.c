#include "log.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <errno.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static void setopts(int s) {
  struct l2cap_options opts = {0};
  int err;
  unsigned int size = sizeof(opts.imtu);

  opts.omtu = opts.imtu = 202;
  log_info("Setting sockopts\n");
  err = setsockopt(s, SOL_BLUETOOTH, BT_RCVMTU, &opts.imtu, size);
  if (!err)
    log_info("Set recv mtu\n");
  else {
    log_info("Unable to set recv mtu. %s (%d)\n", strerror(errno), errno);
  }
  err = setsockopt(s, SOL_BLUETOOTH, BT_SNDMTU, &opts.imtu, size);
  if (!err)
    log_info("Set send mtu\n");
  else {
    log_info("Unable to set send mtu. %s (%d)\n", strerror(errno), errno);
  }
}

int l2cap_connect(char *bd_addr_raw, uint16_t psm) {
  int s = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  int status = -1;

  if (s == -1) {
    log_info("L2CAP: Could not create a socket %s:%u. %s (%d)\n", bd_addr_raw,
             psm, strerror(errno), errno);
  } else {
    log_info("L2CAP: Created socket %s:%u\n", bd_addr_raw, psm);
  }

  struct sockaddr_l2 addr = {0};
  addr.l2_family = AF_BLUETOOTH;
  // See
  // https://elixir.bootlin.com/linux/latest/source/net/bluetooth/l2cap_sock.c#L238
  addr.l2_bdaddr_type = BDADDR_LE_PUBLIC;

  status = bind(s, (struct sockaddr*)&addr, sizeof(addr));
  if (status == 0) {
    log_info("L2CAP socket bound\n");
  } else {
    log_info("L2CAP: Could not bind %s\n", strerror(errno));
  }

  addr.l2_psm = htobs(psm);

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

  if (status == 0) {
    log_info("L2CAP: Connected to %s:%u\n", bd_addr, psm);
  } else {
    log_info("L2CAP: Could not connect to %s:%u. Error %d. %d (%s)\n", bd_addr,
             psm, status, errno, strerror(errno));
  }

  return s;
}
