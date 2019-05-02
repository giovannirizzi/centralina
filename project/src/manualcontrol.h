#ifndef MANUALCONTROL_H
#define MANUALCONTROL_H

#include <sys/types.h>
#include "devices.h"

void send_signal(pid_t pid, SignalType signal, int val);
int interpret_command();

#endif // MANUALCONTROL_H