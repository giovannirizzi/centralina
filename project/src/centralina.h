#ifndef CENTRALINA_H
#define CENTRALINA_H

#include "devices.h"

void list();
int add_device(DEVICE_NAME device);
int delete_device(DEVICE_ID device);
int switch_device(DEVICE_ID device1, DEVICE_ID device2);
int get_info(DEVICE_ID device);

#endif // CENTRALINA_H