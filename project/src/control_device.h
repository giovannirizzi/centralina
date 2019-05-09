#ifndef CONTROL_DEVICES_H
#define CONTROL_DEVICES_H

#include <linux/limits.h>
#include "devices.h"

void add_command(const char** args, size_t n_args);

typedef struct{
    FILE* in;
    FILE* out;
} ChildDevice;

typedef struct{
    ChildDevice children[MAX_CHILDREN];
    int size;
} ChildrenDevices;

int add_child(ChildrenDevices* c, ChildDevice d);
int delete_child(ChildrenDevices* c, int i);

char BIN_PATH[PATH_MAX];
void init_control_device(char *args[], size_t n_args);


#endif // CONTROL_DEVICES_H