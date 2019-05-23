#ifndef CENTRALINA_H
#define CENTRALINA_H

#include "common.h"
#include "control_device.h"

FILE* g_devices_request_stream[MAX_DEVICES] = {NULL};
_Bool is_linked[MAX_DEVICES] = {0};
FILE* g_devices_response_stream;
FILE* g_whois_request_stream;
FILE* g_whois_response_stream;
_Bool g_running;

void init_centralina();
void clean_centralina();
int send_command_to_device(device_id id, const char* command);
int read_device_response(LineBuffer *buffer);

void print_tree(char* tree);
_Bool is_last_sibling(char* node[], int n);

int add_device(DeviceType device);
int delete_device(device_id device);
int link_device(device_id device1, device_id device2);
int switch_device(device_id device, const char* label, const char* pos);
int get_info(device_id device);
void printTree(char* string);

/**
 * Funzioni che validano ed eseguono i comandi della shell
 */
void add_shell_command(const char** args, size_t n_args);
void del_shell_command(const char** args, size_t n_args);
void link_shell_command(const char** args, size_t n_args);
void unlink_shell_command(const char** args, size_t n_args);
void list_shell_command(const char** args, size_t n_args);
void switch_shell_command(const char** args, size_t n_args);
void set_shell_command(const char** args, size_t n_args);
void info_shell_command(const char** args, size_t n_args);
void help_shell_command(const char** args, size_t n_args);
void exit_shell_command(const char** args, size_t n_args);

/**
 * Funzione che implementa il comando per risolvere i PID dei devices
 */
void whois_command(const char** args, size_t n_args);


#endif // CENTRALINA_H