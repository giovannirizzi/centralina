#ifndef CENTRALINA_H
#define CENTRALINA_H

#include "devices.h"

void list();
int add_device(device_name device);
int delete_device(device_id device);
int link_device(device_id device1, device_id device2);
int switch_device(device_id device, char* label, char* pos);
int get_info(device_id device);

#endif // CENTRALINA_H