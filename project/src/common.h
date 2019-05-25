#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#define FIFO_WHOIS_REQUEST "/tmp/centralina/whois_request"
#define FIFO_WHOIS_RESPONSE "/tmp/centralina/whois_response"
#define FIFO_DEVICES_RESPONSE "/tmp/centralina/devices_response"


#define MAX_CHILDREN 50
#define MAX_DEVICES 500
#define MAX_COMMAND_ARGS 5
#define MAX_COMMAND_NAME_LENGTH 30
#define MAX_LABEL_LENGTH 30
#define MAX_REGISTRY_DESC_LENGTH 80

/**
 * Segnali real-time usati per simulare il controllo manuale
 * Per ogni segnale si utilizza anche il valore intero associato
 * ad esso (sival_int) inviato con la syscall sigqueue().
*/
typedef enum{
    SIG_POWER = 0,
    SIG_OPEN,
    SIG_CLOSE,
    SIG_BEGIN,
    SIG_END,
    SIG_DELAY,
    SIG_PERC,
    SIG_TEMP,
    SIG_TICK
} RTSignalType;

/**
 * Definisce la firma di una funzione che converte un intero nella sua rappresentazione in caratteri
 * e anche la sua inversa. Il valore di ritorno è usato per indicare se la conversione ha avuto successo o meno
 */
typedef int (*string_to_int_func_ptr)(const char* value_str, int* value);
typedef int (*int_to_string_func_ptr)(const int value, char* value_str);

typedef struct{
    RTSignalType type;
    int value;
} RTSignal;

//Mapping di un tipo di segnale con una funzione che implementa l'azione da eseguire
typedef void (*action_func_ptr)(const int value);
typedef struct{
    RTSignalType type;
    action_func_ptr exec_command;
} SignalBind;

//Definisce la firma delle funzioni che implementano i comandi
typedef void (*command_func_ptr)(const char** args, const size_t n_args);
typedef struct{
    char command_name[MAX_COMMAND_NAME_LENGTH];
    command_func_ptr validate_and_exec_command;
} CommandBind;
//Associazione di un nome di comando alla funzione che lo implementa

/*Astrazione che viene utilizzata alla base dalla funzione
* getline(), funzione utilizzata in tutto il progetto per
* leggere una linea da un qualsiasi stream.
* Un line buffere deve essere sempre inizzializzato a {NULL, 0} prima
* di utilizzarlo per la lettura.
*/
typedef struct{
    char* buffer;
    size_t length;
} LineBuffer;

typedef struct{
    char* name;
    char* args[MAX_COMMAND_ARGS];
    size_t n_args;
} Command;

#endif //COMMON_H
