#ifndef MANUALCONTROL_H
#define MANUALCONTROL_H

#include <sys/types.h>
#include "devices.h"

void send_signal(pid_t pid, SignalType signal, int val);
int interpret_command();

void whois_command(const char** args, const size_t n_args);
void switch_command(const char** args, const size_t n_args);
void set_command(const char** args, const size_t n_args);
void help_command(const char** args, const size_t n_args);

#endif // MANUALCONTROL_H