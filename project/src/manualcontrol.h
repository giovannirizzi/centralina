#ifndef MANUALCONTROL_H
#define MANUALCONTROL_H

#include <sys/types.h>
#include "devices.h"

void send_signal(pid_t pid, SignalType signal, int val);

#endif // MANUALCONTROL_H