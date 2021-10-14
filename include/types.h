#include <stdint.h>

struct asha_read_only_properties {
  uint8_t version;
  uint8_t device_capabilities;
  uint64_t hi_sync_id;
  uint8_t feature_map;
  uint16_t render_delay;
  uint16_t reserved;
  uint16_t supported_codecs;
};
