#ifndef CENTRALINA_H
#define CENTRALINA_H

#include "devices.h"


int add_device(DeviceType device);
int delete_device(device_id device);
int link_device(device_id device1, device_id device2);
int switch_device(device_id device, const char* label, const char* pos);
int get_info(device_id device);


/*
 * Funzioni che validano ed eseguono i comandi della shell
 */
void add_command(const char** args, const size_t n_args);
void del_command(const char** args, const size_t n_args);
void link_command(const char** args, const size_t n_args);
void list_command(const char** args, const size_t n_args);
void switch_command(const char** args, const size_t n_args);
void info_command(const char** args, const size_t n_args);
void help_command(const char** args, const size_t n_args);
void exit_command(const char** args, const size_t n_args);

#endif // CENTRALINA_H