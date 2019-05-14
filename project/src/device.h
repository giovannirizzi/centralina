#ifndef DEVICES_H
#define DEVICES_H

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
    int value;
    string_to_int_func_ptr convert_value;

} Registry;

typedef struct{

    char label[MAX_LABEL_LENGTH];
    action_func_ptr action;

} Switch;

typedef struct{

    DeviceType type;
    device_id id;
    _Bool running;

    // 0 spento 1 acceso
    int state;
    /*
     * Questi sono puntatori ad altre strutture,
     * possono cambiare in base al tipo di device */
    Registry *records;
    size_t num_records;

    Switch *swtiches;
    size_t num_swtiches;

} DeviceBase;



/**
 * Setta la signal mask del processo, con i segnali real-time
 * passati come parametro
 * @param signal1 segnale real-time da mascherare
 * @param ... altri segnali
 * @return la mask dei segnali passati come argomento
 */
sigset_t set_signal_mask(RTSignalType signal1, ...);


/**
 * Signal function implementation
 *
 */
void power_signal(int value);



/**
 * Function to implement
 */
void info_command(const char** args, size_t n_args);
void del_command(const char** args, size_t n_args);
void setconf_command(const char** args, size_t n_args);
void getconf_command(const char** args, size_t n_args);
void getpid_command(const char** args, size_t n_args);
void gettype_command(const char** args, const size_t n_args);

int handle_device_command(const Command *c, const CommandBind extra_commands[], size_t n);
void init_base_device(char *args[], size_t n_args);
_Bool is_controlled();

const CommandBind BASE_COMMANDS[6];

/**
 * Global vars for all devices
 */

DeviceBase g_device;

FILE* g_curr_out_stream;
FILE* g_fifo_in_stream;
FILE* g_fifo_out_stream;

int g_signal_fd;





#endif // DEVICES_H