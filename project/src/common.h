#ifndef COMMON_H
#define COMMON_H

#define FIFO_WHOIS_REQUEST "/tmp/centralina/whois_request"
#define FIFO_WHOIS_RESPONSE "/tmp/centralina/whois_response"
#define FIFO_DEVICES_RESPONSE "/tmp/centralina/devices_response"

#define MAX_CHILDREN 50
#define MAX_DEVICES 1000
#define MAX_COMMAND_ARGS 5
#define MAX_COMMAND_NAME_LENGTH 30

/**
 * Segnali real-time usati per simulare il controllo manuale
 * Per ogni segnale si utilizza anche il valore intero associato
 * ad esso (sival_int) inviato con la syscall sigqueue().
*/

typedef enum{
    SIG_POWER = 0,
    SIG_OPEN,
    SIG_CLOSE,
    SIG_TIME,
    SIG_DELAY,
    SIG_PERC,
    SIG_TEMP
} RTSignalType;

typedef struct{
    RTSignalType type;
    int value;
} RTSignal;

typedef void (*signal_func_ptr)(const int value);
typedef struct{
    RTSignalType type;
    signal_func_ptr exec_command;
} SignalBind;


typedef void (*command_func_ptr)(const char** args, const size_t n_args);
typedef struct{
    char command_name[MAX_COMMAND_NAME_LENGTH];
    command_func_ptr validate_and_exec_command;
} CommandBind;

typedef struct{
    char* name;
    size_t len_name;
    char* args[MAX_COMMAND_ARGS];
    size_t n_args;
} Command;

#endif //COMMON_H
