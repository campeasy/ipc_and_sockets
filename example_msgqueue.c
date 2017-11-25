// ------------------------------------------------------------
// Salvatore Campisi
// C PROGRAMMING
// Anno Accademico 2017/2018
// Argomento: Unix Programming - IPC: Coda di Messaggi
// ------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>

#define KB 1024 // Definizione Kilobyte.

typedef struct {
    long type;
    char text[KB];
} pacchetto;

int main(int argc, char *argv[]){
    int ds_coda;
    key_t chiave_coda = 6;

    // Creazione Coda:
    ds_coda = msgget(chiave_coda, IPC_CREAT | IPC_EXCL | 0777);
    if(ds_coda == -1) exit(1);
    
    // Codice FIGLIO 1;
    pid_t firstpid = fork();
    if(firstpid == 0){
        pacchetto mypacket;
        mypacket.type = 1;
        strncpy(mypacket.text, "Hello World!", KB);

        if(msgsnd(ds_coda, &mypacket, strlen(mypacket.text)+1, IPC_NOWAIT) == -1){ perror("msgsnd"); exit(1); }
        
        exit(0);
    }
    if(firstpid == -1) exit(1);
    
    // Codice FIGLIO 2:
    pid_t secondpid = fork();
    if(secondpid == 0){
        waitpid(firstpid,NULL,0); // Attendiamo che il messaggio venga prima caricato.
        pacchetto miopacc;
        miopacc.type = 1;
        
        if(msgrcv(ds_coda, &miopacc, sizeof(miopacc)-sizeof(long), 0, 0) == -1){ perror("msgrcv"); exit(1); }
        
        printf("Il messaggio ricevuto è: %s \n",miopacc.text);
        exit(0);
    }
    if(secondpid == -1) exit(1);
    
    // Attendiamo che i processi abbiano finito:
    waitpid(firstpid,NULL,0);
    waitpid(secondpid,NULL,0);
    // Distruzione Coda:
    msgctl(ds_coda, IPC_RMID, NULL);
    
    exit(0);
}
