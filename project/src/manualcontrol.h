#ifndef MANUALCONTROL_H
#define MANUALCONTROL_H

#include "common.h"

typedef int device_id;

/*Associa un label (che può essere uno switch oppure un registro)
 * ad un segnale e ad una funzione per convertire il value da stringa a intero.
*/
typedef struct {
    char label[20];
    RTSignalType signal;
    string_to_int_func_ptr string_to_signal_value;
} SignalMapping;

/**
 * Invia una richiesta di whois usando il servizio
 * del controller tramite la sua fifo
 * @param id l'id del dispositivo da risolvere
 * @return il pid del dispositivo che ha ricevuto in risposta dalla fifo,
 * -1 se non è presente un dispositivo con tale id, -2 se il controller è offline
 */
pid_t whois(device_id id);

/**
 * Invia un segnale real-time con un intero
 * @param pid il pid del processo dove inviare il segnale
 * @param signal il tipo di segnale
 * @param val il valore intero da inviare
 */
void send_signal(pid_t pid, RTSignalType signal, int val);

/**
 * Scorre l'array in cerca del label
 * @return la posizione nell'array mappings, -1 se non è stato trovato un mapping nell'array
 */
int get_signal_mapping(const char* label, const SignalMapping mappings[], size_t n);


//Implementano i comandi
void whois_command(const char** args, size_t n_args);
void switch_command(const char** args, size_t n_args);
void set_command(const char** args, size_t n_args);
void help_command(const char** args, size_t n_args);

#endif // MANUALCONTROL_H