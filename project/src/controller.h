#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "common.h"
#include "control_device.h"

_Bool g_running;

//Stream per comandare ogni device nel sistema.
FILE* g_devices_request_stream[MAX_DEVICES] = {NULL};
FILE* g_devices_response_stream;

//Stream per il servizo di risoluzione dei pid dato l'id.
FILE* g_whois_request_stream;
FILE* g_whois_response_stream;


//Inizializza l'intero sistema e avvia il dispositivo controller
void init_controller();
//Elimina tutti i dispositivi dal sistema
void clean_controller();

//Stampa a video in modo migliore le risoste dei dispositivi
void pretty_print(const char* feedback);
//Esegue la fork e la exec di un nuovo dispositivo e lo aggiunge al sistema
int add_device(DeviceType device);

/**
 * Prende una stringa che contiene l'albero di dispositivi con la loro configurazione e ricorsivamente
 * manda il comando add ai control devices per ricostruire l'albero.
 * Serve per il comando link.
 * @param parent è l'id del dispositivo di controllo dove mandare il comando add
 */
void add_child_devices_recursive(int parent, char **start, char **end, LineBuffer *buffer);

int delete_device(device_id device);

//Implementa la logica del comando link
int link_device(device_id device1, device_id device2);

/**
 * Funzione helper per inviare un comando ad un device specifico.
 * Se il canale di comunicazione è chiusto (quindi il device è terminato) elimina automaticamente
 * il dispositivo dal sistema
 * @return 0 se il comando è stato inviato, -1 se invece il dispositivo è stato eliminato
 */
int send_command_to_device(device_id id, const char* command);

/**
 * Legge lo stream dove i dispositivi inviano le risposte hai comandi.
 * Rimane in attesa per 1 secondo poi restituisce anche se non c'è nulla
 * da leggere
 * @return il numero di byte letti, 0 se il timeout è avvenuto
 */
int read_device_response(LineBuffer *buffer);

/**
 * Funzione che implementa il comando per risolvere i PID dei devices
 */
void whois_command(const char** args, size_t n_args);

/**
 * Funzioni che implementano i comandi della shell
 */
void add_shell_command(const char** args, size_t n_args);
void del_shell_command(const char** args, size_t n_args);
void link_shell_command(const char** args, size_t n_args);
void list_shell_command(const char** args, size_t n_args);
void switch_shell_command(const char** args, size_t n_args);
void set_shell_command(const char** args, size_t n_args);
void info_shell_command(const char** args, size_t n_args);
void help_shell_command(const char** args, size_t n_args);
void exit_shell_command(const char** args, size_t n_args);

/*
 * Funzioni che implementano i comandi extra e la logica
 * dell'interruttore generale del dispositivo controller
 */
void devicecommand_controller_command(const char **args, size_t n_args);
void getinfo_controller_command(const char** args, size_t n_args);
void switch_state_controller(int state);
void switch_controller_command(const char** args, const size_t n_args);



#endif // CONTROLLER_H