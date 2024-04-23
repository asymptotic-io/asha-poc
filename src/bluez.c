#include "log.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <errno.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// Set based on the spec: https://source.android.com/devices/bluetooth/asha
#define MTU 167
#define CREDITS 8

static void set_options(int s, int mtu) {
  struct l2cap_options opts = {0};
  int err = 0;
  unsigned int value, size = sizeof(opts.imtu);

  opts.omtu = opts.imtu = mtu;
  opts.mode = BT_MODE_LE_FLOWCTL;
  log_info("Setting sockopts\n");

  err = setsockopt(s, SOL_BLUETOOTH, BT_SNDMTU, &opts.omtu, size);
  if (err) {
    log_info("Unable to set send mtu. %s (%d)\n", strerror(errno), errno);
  } else {
    log_info("Set send mtu\n");
  }

  err = setsockopt(s, SOL_BLUETOOTH, BT_RCVMTU, &opts.imtu, size);
  if (err) {
    log_info("Unable to set recv mtu. %s (%d)\n", strerror(errno), errno);
  } else {
    log_info("Set recv mtu\n");
  }

  err = setsockopt(s, SOL_BLUETOOTH, BT_MODE, &opts.mode, sizeof(opts.mode));
  if (err) {
    log_info("Unable to set flowctl mode %s (%d)\n", strerror(errno), errno);
  } else {
    log_info("Set flowctl mode\n");
  }

#if 0
  value = mtu * CREDITS;
  err = setsockopt(s, SOL_SOCKET, SO_RCVBUF, &value, sizeof(value));
  if (err) {
    log_info("Unable to set rcvbuf %s (%d)\n", strerror(errno), errno);
  } else {
    log_info("Set rcvbuf\n");
  }

  value = mtu * CREDITS;
  err = setsockopt(s, SOL_SOCKET, SO_SNDBUF, &value, sizeof(value));
  if (err) {
    log_info("Unable to set sndbuf %s (%d)\n", strerror(errno), errno);
  } else {
    log_info("Set sndbuf\n");
  }
#endif
}

static void strsub(char *str, char c, char replacement, int len) {
  for (size_t i = 0; i < len; i++) {
    if (str[i] == c) {
      str[i] = replacement;
    }
  }
}

#define ADDR_LENGTH 30

int l2cap_connect(char *bd_addr_raw, uint16_t psm) {
  int s = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  int status = -1;

  if (s == -1) {
    log_info("L2CAP: Could not create a socket %s:%u. %s (%d)\n", bd_addr_raw,
             psm, strerror(errno), errno);
    return -1;
  }

  log_info("L2CAP: Created socket %s:%u\n", bd_addr_raw, psm);

  struct sockaddr_l2 addr = {0};
  char bd_addr[ADDR_LENGTH];
  strncpy(bd_addr, bd_addr_raw, ADDR_LENGTH);

  addr.l2_family = AF_BLUETOOTH;
  addr.l2_bdaddr_type = BDADDR_LE_PUBLIC;

  // Currently need to bind before connect to workaround an issue where the
  // addr type is incorrectly set otherwise

  status = bind(s, (struct sockaddr *)&addr, sizeof(addr));

  if (status < 0) {
    log_info("L2CAP: Could not bind %s\n", strerror(errno));
    return -1;
  }

  log_info("L2CAP: socket bound\n");

  addr.l2_psm = htobs(psm);
  strsub(bd_addr, '_', ':', ADDR_LENGTH);
  str2ba(bd_addr, &addr.l2_bdaddr);
  set_options(s, MTU);

  status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

  if (status == 0) {
    log_info("L2CAP: Connected to %s:%u\n", bd_addr, psm);
  } else {
    log_info("L2CAP: Could not connect to %s:%u. Error %d. %d (%s)\n", bd_addr,
             psm, status, errno, strerror(errno));
    return -1;
  }

  return s;
}
