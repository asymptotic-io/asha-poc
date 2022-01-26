#include <stdint.h>

#ifndef ASHA_SUPPORT_TYPES_H
#define ASHA_SUPPORT_TYPES_H

struct ha_device_capabilities {
  uint8_t side : 1;
  uint8_t type : 1;
  uint8_t reserved : 6;
};

struct feature_map {
  uint8_t coc_streaming_supported : 1;
  uint8_t reserved : 7;
};

struct supported_codecs {
  uint8_t reserved : 1;
  uint8_t g722 : 1;
  uint16_t also_reserved : 14;
};

struct ha_properties {
  uint8_t version;
  struct ha_device_capabilities device_capabilities;
  uint64_t hi_sync_id;
  struct feature_map feature_map;
  uint16_t render_delay;
  uint16_t reserved;
  struct supported_codecs supported_codecs;
};

#define is_right(properties) ((properties).device_capabilities.side)
#define is_left(properties) (!(properties).device_capabilities.side)

#define is_monoaural(properties) (!properties.device_capabilities.type)
#define is_binaural(properties) (properties.device_capabilities.type)

#define hi_sync_id(properties) (properties.hi_sync_id)
#define render_delay(properties) (properties.render_delay)

#define is_supported(properties)                                               \
  (properties.feature_map.coc_streaming_supported &                            \
   properties.supported_codecs.g722)

#define DBUS_PATH_LIMIT 100
struct ha_dbus_paths {
  char device_path[DBUS_PATH_LIMIT];
  char service_path[DBUS_PATH_LIMIT];

  char read_only_properties_path[DBUS_PATH_LIMIT];
  char audio_control_point_path[DBUS_PATH_LIMIT];
  char audio_status_path[DBUS_PATH_LIMIT];
  char volume_path[DBUS_PATH_LIMIT];
  char le_psm_out_path[DBUS_PATH_LIMIT];
};

enum ConnectionStatus { DISCONNECTED = 0, CONNECTED = 1 };

struct ha_device {
  struct ha_dbus_paths dbus_paths;

  // fields containing data fetched from the paths above
  struct ha_properties properties;
  uint16_t le_psm;

  // state information
  enum ConnectionStatus connection_status;
  uint8_t sequence_counter;
  int socket;

  // streaming fields
  void *buffer;
  int source;    // source fd to read from locally to write to the stream
  int sdulen;    // sdu len used for the l2cap connection
  int iteration; // incremented on each streaming iteration
};

struct ha_pair {
  struct ha_device left;
  struct ha_device right;
};

#define MAX_HA_PAIRS 20

struct ha_pairs {
  struct ha_pair pairs[MAX_HA_PAIRS];
  uint8_t length;
};

#endif
