#ifndef CONTROL_DEVICES_H
#define CONTROL_DEVICES_H

#include <linux/limits.h> //per PATH_MAX
#include "device.h"

typedef struct{
    FILE* in;
    FILE* out;
} ChildDevice;

typedef struct{
    ChildDevice children[MAX_CHILDREN];
    int size;
} ChildrenDevices;

/* Ogni control device ha un vettore "di dispositivi figli",
 * per ogni dispositivo figlio si memorizza solo il canale
 * di comunicazione per inviare comandi e leggerne la risposta
 */
ChildrenDevices g_children_devices;

/*
 * Implementazione del comando add comune a tutti i control devices.
 * Aggiunge un dispositivo figlio e ne setta la configurazione
 * formato comando: add <id>|<type>|<state>|<registry>=<value>&<registry2>..
 */
void add_command(const char** args, size_t n_args);

/**
 * Fa la fork e la exec di un nuovo dispositivo e lo aggiunge
 * al vettore dei dispositivi figli
 * @param id l'id da assegnare al nuovo dispositivo
 * @param d_type il tipo dispositivo da aggiungere
 * @return la posizione dove è stato aggiunto nel vettore dei dispositivi,
 * -1 se c'stato un errore
 */
int add_child_device(int id, DeviceType d_type);

/*
 * Elimina il dispositivo figlio in posizione <child>,
 * (il processo del dispositivo termina perché il suo stdin viene chiuso)
 */
void delete_child_device(int child);

/*
 * Inizializza il dispositivo di controllo
 */
void init_control_device(char *args[], size_t n_args);

/*
 * Rilascia le risorse e elimina tutti i dispositivi figli
 */
void clean_control_device();

/*
 * Handler del segnale SIGCHILD per fare la wait sui figli
 * che sono terminati
 */
void sigchild_handler(int signum);


/**
 * Funzione per comandare un dispositivo figlio, se il figlio
 * non è più raggiungibile lo elimnia
 * @return 0 se il comando è stato inviato, -1 se il figlio è stato eliminato
 */
int send_command_to_child(int child, const char* command);


/**
 * Legge il canale di risposta del figlio
 * @param buffer buffer dove scrivere la risposta
 * @return il numero di byte letti, se < 0 si è verificato un errore
 */
int read_child_response(int child, LineBuffer *buffer);

//Funzioni per gestire l'array
int add_child(ChildrenDevices* c, ChildDevice d);
int delete_child(ChildrenDevices* c, int i);


#endif // CONTROL_DEVICES_H