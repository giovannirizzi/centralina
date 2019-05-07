#ifndef DEVICES_H
#define DEVICES_H

#define MAX_DEVICES 1000
#define MAX_COMMAND_ARGS 5

typedef int device_id;
typedef char* device_name;

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

    DeviceType type;
    device_id id;
    int singal_fd;

    // 0 spento 1 acceso
    int state;

    /*
     * Questi sono puntatori ad altre strutture,
     * possono cambiare in base al tipo di device */
    const void* interruttori_struct;
    const void* data_struct;

} DeviceBase;

/* Segnali real-time usati per simulare il controllo manuale
 * Per ogni segnale si utilizza anche il valore intero associato
 * ad esso (sival_int) inviato con la syscall sigqueue().
*/
typedef enum{
    SIG_POWER = 0,
    SIG_OPEN,
    SIG_CLOSE
} SignalType;


/**
 * Setta la signal mask del processo, con i segnali real-time
 * passati come parametro
 * @param signal1 segnale real-time da mascherare
 * @param ... altri segnali
 * @return la mask dei segnali passati come argomento
 */
sigset_t set_signal_mask(SignalType signal1, ...);


/**
 * Signal function implementation
 *
 */


typedef void (*command_func_ptr)(const char** args, const size_t n_args);
typedef void (*singal_func_ptr)(const int value);

typedef struct{
    char command_name[20];
    command_func_ptr validate_and_exec_command;
} CommandBind;

typedef struct{
    char* name;
    size_t len_name;
    char* args[MAX_COMMAND_ARGS];
    size_t n_args;
} Command;


void power_signal(const int value);

/**
 * Global var for all devices
 */
DeviceBase device_data;

/**
 * Function to implement
 */
void info_command(const char** args, const size_t n_args);
void del_command(const char** args, const size_t n_args);
void setconf_command(const char** args, const size_t n_args);
void getconf_command(const char** args, const size_t n_args);
void getpid_command(const char** args, const size_t n_args);

int handle_device_command(const Command *c, const CommandBind custom_commands[], const size_t n);
void init_device(device_id id, int signalfd);

CommandBind base_commands[5];

FILE* command_output;
FILE* fifo_request;
FILE* fifo_response;





#endif // DEVICES_H