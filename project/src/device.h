#ifndef DEVICES_H
#define DEVICES_H

#define INV_ARGS "INVALID ARGS"
#define INV_SWITCH_STATE "INVALID SWITCH STATE"
#define INV_SWITCH "INVALID SWITCH NAME"
#define INV_SET_VALUE "INVALID SET VALUE"
#define INV_REG "INVALID REGISTRY NAME"
#define INV_COMMAND "UNKNOWN COMMAND"
#define ERR_REG_UNSETTABLE "REGISTER NOT SETTABLE"
#define OK_NO_CHANGES "SWITCH ALREADY_SET"
#define OK_DONE "DONE"
#define ERR "UNKNOWN ERROR"

#include <signal.h>
#include "common.h"


typedef int device_id;

//Numeri negativi per i devices di controllo e positivi per iterazione
typedef enum {
    TIMER = -3,
    HUB = -2,
    CENTRALINA = -1,
    INVALID_TYPE = 0,
    BULB = 1,
    WINDOW = 2,
    FRIDGE = 3
} DeviceType;

typedef struct{

    char label[MAX_LABEL_LENGTH];
    char description[MAX_REGISTRY_DESC_LENGTH];
    int value;
    string_to_int_func_ptr convert_value;
    int_to_string_func_ptr format_value;
    _Bool is_settable;

} Registry;

typedef struct{

    char label[MAX_LABEL_LENGTH];
    action_func_ptr action;

} Switch;

typedef struct{

    DeviceType type;
    device_id id;
    // 0 spento 1 acceso
    int state;
    /*
     * Questi sono puntatori ad altre strutture,
     * possono cambiare in base al tipo di device */
    Registry *records;
    size_t num_records;

    Switch *switches;
    size_t num_switches;

} DeviceData;

const CommandBind BASE_COMMANDS[11];

/**
 * Global vars for all devices
 */
DeviceData g_device;

FILE* g_fifo_in_stream;
FILE* g_fifo_out_stream;
FILE* g_curr_out_stream;
_Bool g_running;
RTSignal g_curr_signal;


/*
 * Function to implement
 */
void getinfo_command(const char **args, size_t n_args);
void del_command(const char** args, size_t n_args);
void setconf_command(const char** args, size_t n_args);
void getconf_command(const char** args, size_t n_args);
void getpid_command(const char** args, size_t n_args);
void gettype_command(const char** args, size_t n_args);
void iscontrolled_command(const char** args, size_t n_args);
void switch_command(const char** args, size_t n_args);
void set_command(const char** args, size_t n_args);
void getrealtype_command(const char** args, size_t n_args);
void gettree_command(const char** args, size_t n_args);

int handle_device_command(const Command *c, const CommandBind extra_commands[], size_t n);
void init_base_device(char *args[], size_t n_args);
_Bool is_controlled();
void device_loop(const SignalBind signal_bindings[], size_t n_sb, const CommandBind extra_commands[],
                 size_t n_dc);
void clean_base_device();
int send_response(char* response, ...);
int get_records_string(char* buffer);
int set_records_from_string(char *records);

/**
 * Setta la signal mask del processo, con i segnali real-time
 * passati come parametro
 * @param signal1 segnale real-time da mascherare
 * @param ... altri segnali
 * @return la mask dei segnali passati come argomento
 */
sigset_t set_signal_mask(RTSignalType signal1, ...);

void signal_handler(int sig, siginfo_t *info, void *context);





#endif // DEVICES_H