#ifndef CENTRALINA_H
#define CENTRALINA_H

#include "common.h"
#include "control_device.h"


FILE* devices_in_stream[MAX_DEVICES] = {NULL};
FILE* devices_response_stream;

void init_centralina();

int add_device(DeviceType device);
int delete_device(device_id device);
int link_device(device_id device1, device_id device2);
int switch_device(device_id device, const char* label, const char* pos);
int get_info(device_id device);

/**
 * Funzioni che validano ed eseguono i comandi della shell
 */
void add_shell_command(const char** args, size_t n_args);
void del_shell_command(const char** args, size_t n_args);
void link_shell_command(const char** args, size_t n_args);
void list_shell_command(const char** args, size_t n_args);
void switch_shell_command(const char** args, size_t n_args);
void info_shell_command(const char** args, size_t n_args);
void help_shell_command(const char** args, size_t n_args);
void exit_shell_command(const char** args, size_t n_args);

/**
 * Funzione che implementa il comando per risolvere i PID dei devices
 */
void whois_command(const char** args, size_t n_args);


#endif // CENTRALINA_H