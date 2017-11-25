// ------------------------------------------------------------
// Salvatore Campisi
// C PROGRAMMING
// Anno Accademico 2017/2018
// Argomento: Unix Programming - IPC: Semafori
// ------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>

// Creazione operazioni WAIT - SIGNAL dato uno specifico semaforo:
int WAIT(int descrittore_semaforo, int posizione_semaforo){
    struct sembuf wait_op [1] = {{posizione_semaforo,-1,0}};
    return semop(descrittore_semaforo,wait_op,1);
}

int SIGNAL(int descrittore_semaforo, int posizione_semaforo){
    struct sembuf signal_op [1] = {{posizione_semaforo,+1,0}};
    return semop(descrittore_semaforo,signal_op,1);
}

int main(){
    // Creazione del semaforo:
    key_t chiave_semaforo = IPC_PRIVATE;
    int descrittore_semaforo = semget(chiave_semaforo, 1, IPC_CREAT | IPC_EXCL | 0770);
    if(descrittore_semaforo == -1){ perror("semget"); exit(1); }
    
    // Inizializzazione del semaforo:
    if(semctl(descrittore_semaforo, 0, SETVAL, 1) == -1){ perror("semctl"); exit(1); }
    
    pid_t firstpid = fork();
    if(firstpid == -1){ perror("fork"); exit(1); }
    if(firstpid == 0){
        if(WAIT(descrittore_semaforo,0) == -1){ perror("semop"); exit(1); }
            printf("INIZIO SEZIONE CRITICA PROCESSO 1 .. \n");
            for(int i=0; i<100000; i++){}
            printf("FINE SEZIONE CRITICA PROCESSO 1 .. \n\n");
        if(SIGNAL(descrittore_semaforo,0) == -1){ perror("semop"); exit(1); }
        
        exit(0);
    }
    
    pid_t secondpid = fork();
    if(secondpid == -1){ perror("fork"); exit(1); }
    if(secondpid == 0){
        if(WAIT(descrittore_semaforo,0) == -1){ perror("semop"); exit(1); }
            printf("INIZIO SEZIONE CRITICA PROCESSO 2 .. \n");
            for(int i=0; i<100000; i++){}
            printf("FINE SEZIONE CRITICA PROCESSO 2 .. \n\n");
        if(SIGNAL(descrittore_semaforo,0) == -1){ perror("semop"); exit(1); }
        
        exit(0);
    }
    
    pid_t thirdpid = fork();
    if(thirdpid == -1){ perror("fork"); exit(1); }
    if(thirdpid == 0){
        if(WAIT(descrittore_semaforo,0) == -1){ perror("semop"); exit(1); }
        printf("INIZIO SEZIONE CRITICA PROCESSO 3 .. \n");
        for(int i=0; i<100000; i++){}
        printf("FINE SEZIONE CRITICA PROCESSO 3 .. \n\n");
        if(SIGNAL(descrittore_semaforo,0) == -1){ perror("semop"); exit(1); }
        
        exit(0);
    }
    
    wait(NULL); // Attende che termini ALMENO UN figlio. (Qualsiasi)
    wait(NULL); // Attende che termini ALMENO UN figlio. (Qualsiasi)
    wait(NULL); // Attende che termini ALMENO UN figlio. (Qualsiasi)
    
    // Distruzione del semaforo:
    semctl(descrittore_semaforo, 0, IPC_RMID, 0);
    
    exit(0);
}
