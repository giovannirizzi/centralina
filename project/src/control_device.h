#ifndef CONTROL_DEVICES_H
#define CONTROL_DEVICES_H

#include <linux/limits.h>
#include "device.h"

void add_command(const char** args, size_t n_args);

int add_child_device(int id, DeviceType d_type);
void delete_child_device(int child);
void clean_control_device();
void sigchild_handler(int signum);

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
int send_command_to_child(int child, const char* command);
int read_child_response(int child, LineBuffer *buffer);

void init_control_device(char *args[], size_t n_args);

ChildrenDevices children_devices;


#endif // CONTROL_DEVICES_H