#ifndef MANUALCONTROL_H
#define MANUALCONTROL_H

#include "common.h"

typedef int device_id;

typedef struct {
    char label[20];
    RTSignalType signal;
} SignalMapping;

pid_t whois(device_id id);
void send_signal(pid_t pid, RTSignalType signal, int val);
int get_signal_mapping(const char* label, const SignalMapping mappings[], size_t n);

void whois_command(const char** args, size_t n_args);
void switch_command(const char** args, size_t n_args);
void set_command(const char** args, size_t n_args);
void help_command(const char** args, size_t n_args);

#endif // MANUALCONTROL_H