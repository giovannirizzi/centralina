Battan Omar 188346
Menapace Marco 185023
Rizzi Giovanni 195013


manualcontrol

Eseguibile per simulare il controllo manuale dei dispositivi.
Utilizza i segnali real-time (usufruendo anche del valore intero associato) come IPC verso i dispositivi.
Per riferirsi ad un particolare dispositivo è possibile utilizzare il suo valore intero associato, l'id.
Il processo quindi risolve l'id traducendolo in PID interrogando la centralina mediante l'utilizzo di una named pipe.
Più in dettaglio il processo scrive il comando whois <id> su una named pipe (/tmp/centralina/whois) e la centralina
risponde con il PID associato oppure -1 se non esiste.

Utilizzo:
    
    manualcontrol whois <id>    -   Restituisce il PID del dispositivo che ha quell'id.
    manualcontrol switch <id/PID> <label> <pos/value>    -    Invia il segnale associato al label e alla pos/value al dispositivo con quell'id o PID.
    
    
    
    
centralina

Il processo rimane in ascolto di comandi provenienti da stdin e dalla named pipe per il servizio di risoluzione degli id.
