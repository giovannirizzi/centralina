#ifndef MANUALCONTROL_H
#define MANUALCONTROL_H

#include "common.h"

typedef int device_id;

// aggiungere a SignalMapping una puntatore a funzione int func(char*, *int) (come string_to_int) che converte il secondo argomento 
// del comando set e switch ad un intero, il valore di ritorno serve per controllare se la conversione è andata a buon fine.
// I switch label mappings avranno tutti la funzione &string_to_device_state

typedef int (*string_to_int_func_ptr)(const char* label, int* n);
typedef struct {
    char label[20];
    RTSignalType signal;
    string_to_int_func_ptr string_to_signal_value;
} SignalMapping;

pid_t whois(device_id id);
void send_signal(pid_t pid, RTSignalType signal, int val);
int get_signal_mapping(const char* label, const SignalMapping mappings[], size_t n);

void whois_command(const char** args, size_t n_args);
void switch_command(const char** args, size_t n_args);
void set_command(const char** args, size_t n_args);
void help_command(const char** args, size_t n_args);

#endif // MANUALCONTROL_H