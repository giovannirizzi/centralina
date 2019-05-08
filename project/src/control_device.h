#ifndef CONTROL_DEVICES_H
#define CONTROL_DEVICES_H

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


#endif // CONTROL_DEVICES_H