#ifndef DEVICES_H
#define DEVICES_H

#define INV_ARGS "INVALID ARGS"
#define INV_SWITCH_STATE "INVALID SWITCH STATE"
#define INV_SWITCH "INVALID SWITCH NAME"
#define INV_SET_VALUE "INVALID SET VALUE"
#define INV_REG "INVALID REGISTRY NAME"
#define INV_ID "INVALID ID"
#define INV_COMMAND "UNKNOWN COMMAND"
#define ERR_REG_UNSETTABLE "REGISTER NOT SETTABLE"
#define OK_NO_CHANGES "SWITCH ALREADY_SET"
#define OK_DONE "DONE"
#define ERR "UNKNOWN ERROR"
#define ERR_CONTROLLER_OFF "CONTROLLER OFF"
#define ERR_NO_DEVICES "NO DEVICES"

#include <signal.h>
#include "common.h"


typedef int device_id;

//Numeri negativi per i devices di controllo e positivi per iterazione
typedef enum {
    TIMER = -3,
    HUB = -2,
    CONTROLLER = -1,
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

    Registry *records;
    size_t num_records;

    Switch *switches;
    size_t num_switches;

} DeviceData;


const CommandBind BASE_COMMANDS[13];


// Ogni dispositivo ha le seguenti variabili globali

DeviceData g_device;

//input dei comandi di "sistema"
FILE* g_fifo_in_stream;

//output risposte
FILE* g_fifo_out_stream;

//indica lo stream corrente dove scrivere la risposta (varia tra stdout e fifo e NULL)
FILE* g_curr_out_stream;

/* Ultimo segnale ricevuto.
 * g_curr_signal.type può essere -1, in quel caso non c'è un nuovo segnale
 * da gestire
 */
RTSignal g_curr_signal;

_Bool g_running;


/*
 * Funzioni che implementano i comandi base, alcune delle
 * implementazioni non si tovano in device.c ma in control_device
 * e interaction_device. Questo perché le implementazioni differiscono
 * in base al device
 */

//Resituisce le info del dispositivo in formato testuale
void getinfo_command(const char **args, size_t n_args);

//Fa terminare il dispositivo
void del_command(const char** args, size_t n_args);

/*Setta stato e registri settabili del dispositivo,
 formato argomento: id|<registro>=<valore>&<registro2>=<valore> */
void setconf_command(const char** args, size_t n_args);

//Risponde con id|type|state|<records>
void getconf_command(const char** args, size_t n_args);

//Risponde con il proprio pid
void getpid_command(const char** args, size_t n_args);

//Risponde con il tipo del device in stringa
void gettype_command(const char** args, size_t n_args);

//yes se è controllato no altrimenti
void iscontrolled_command(const char** args, size_t n_args);

//switch <nome_switch> <on/off>
void switch_command(const char** args, size_t n_args);

//set <nome_registro_settabile> <on/off>
void set_command(const char** args, size_t n_args);

/* Risponde con il vero tipo del dispositivo in intero
 (se un dispositvo di controllo non controlla nessun dispositvo
 allora risponde con 0 (ivalid_type) */
void getrealtype_command(const char** args, size_t n_args);

//Risponde con l'albero del dispositivo, il node è id|type|
void gettree_command(const char** args, size_t n_args);

//Risponde con l'id
void getid_command(const char **args, size_t n_args);

//Risponde con lo stato in intero
void getstate_command(const char **args, size_t n_args);


/*
 * Chiama la giusta funzione in base al comando passato come parametro
 * È possibile definire comandi extra.
 */
int handle_device_command(const Command *c, const CommandBind extra_commands[], size_t n);

/*
 * Inizzializza il dispositovo: apre la fifo per ricevere i comandi di sistema
 * (se gli viene passato come argomento l'id) e maschera i segnali.
 */
void init_base_device(char *args[], size_t n_args);

//true se il dispositivo ha un controllore
_Bool is_controlled();

/**
 * Legge e processa i comandi ed i segnali in input
 * @param signal_bindings Array di segnali da "ascoltare" con la relativa azione da esesguire
 * @param extra_commands Comandi extra del dispositivo da processare
 */
void device_loop(const SignalBind signal_bindings[], size_t n_sb, const CommandBind extra_commands[],
                 size_t n_dc);

void clean_base_device();

//Funziona come fprintf ed invia l'output a g_curr_out_stream e fa il flush
int send_response(char* format, ...);

//Restituisce i registri settabili del dispositivo nel formato: <registry>=<value>&<registr2>=..
int get_records_string(char* buffer);

//Setta il valore dei registri dati in input
int set_records_from_string(char *records);

//Signal handler comune che salva il segnale con il suo valore in g_curr_signal
void signal_handler(int sig, siginfo_t *info, void *context);

/**
 * Setta la signal mask del processo, con i segnali real-time
 * passati come parametro e per ciascuno registra anche
 * il signal_handler comune
 * @param signal1 segnale real-time da mascherare
 * @param ... altri segnali
 * @return la mask dei segnali passati come argomento
 */
sigset_t set_signal_mask(RTSignalType signal1, ...);


#endif // DEVICES_H