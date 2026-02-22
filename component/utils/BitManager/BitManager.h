#ifndef BIT_MANAGER_H
#define BIT_MANAGER_H
#include "Common.h"


typedef enum {
  MESSAGE_NONE = -1,
  MESSAGE_SENSOR_USED_OTHER_PORT = 0,
  MESSAGE_SENSOR_NOT_INITIALIZED,
  MESSAGE_PORT_SELECTED,
  MESSAGE_PORT_NOT_SELECTED,
} Message_t;


extern const uint8_t **imageManager[];
extern const char **ManagerText[];
extern const char *MessageText[];

#endif