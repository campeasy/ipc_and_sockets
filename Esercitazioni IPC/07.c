// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define KB        1024
#define N         10
#define SHM_DIM   80
#define P_SEM     0
#define M_SEM     1
#define O_SEM     2

// Definizione di un record [numero, tipo]:
typedef struct{
    int numero; // 4 BYTE
    int tipo;   // 4 BYTE
} pacchetto;

int WAIT(int sem_des, int sem_pos){
    struct sembuf wait_ops[1] = {{sem_pos,-1,0}};
    return semop(sem_des, wait_ops, 1);
}
int SIGNAL(int sem_des, int sem_pos){
    struct sembuf signal_ops[1] = {{sem_pos,+1,0}};
    return semop(sem_des, signal_ops, 1);
}

void closing_function(char *err, int mem_des, int sem_des){
    if(err != NULL) perror(err);
    if(mem_des != -1) shmctl(mem_des, IPC_RMID, NULL);
    if(sem_des != -1) semctl(sem_des, 0, IPC_RMID, 0);
    return;
}

int main(int argc, char *argv[]){
    if(argc < 3) exit(1);
    char *file_pathname = argv[1];
    int mod_number = atoi(argv[2]);
    
    // Creazione Memoria Condivisa:
    int descrittore_memoria = shmget(IPC_PRIVATE, SHM_DIM, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_memoria == -1){
        closing_function("shmget", -1, -1);
        exit(1);
    }
    // Annessione Memoria Condivisa:
    pacchetto *pointer = (pacchetto *) shmat(descrittore_memoria, NULL, 0);
    if(pointer == NULL){
        closing_function("shmat", descrittore_memoria, -1);
        exit(1);
    }
    // Inizializzazione Memoria Condivisa:
    for(int i=0; i<N; i++){
        pointer[i].numero = -1;
        pointer[i].tipo = -1;
    }
    // Creazione Semaforo:
    int descrittore_semaforo = semget(IPC_PRIVATE, 3, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_semaforo == -1){
        closing_function("semget", descrittore_memoria, -1);
        exit(1);
    }
    // Inizializzazione Semafori:
    if(semctl(descrittore_semaforo, P_SEM, SETVAL, 1) == -1){
        closing_function("semctl", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    if(semctl(descrittore_semaforo, M_SEM, SETVAL, 0) == -1){
        closing_function("semctl", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    if(semctl(descrittore_semaforo, O_SEM, SETVAL, 0) == -1){
        closing_function("semctl", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    
    // CODICE FIGLIO M:
    pid_t m_pid = fork();
    if(m_pid == -1){ closing_function("fork", descrittore_memoria, descrittore_semaforo); exit(1); }
    if(m_pid == 0){
        int terminator_token = 0;
        
        while(1){
            WAIT(descrittore_semaforo, M_SEM);
            // Scansionamento Buffer:
            for(int i=0; i<N; i++){
                // Controlliamo che non sia un terminatore:
                if(pointer[i].tipo != -1){
                    pointer[i].numero = pointer[i].numero % mod_number;
                    pointer[i].tipo = 1;
                }
                else terminator_token = 1;
            }
            // Il file è terminato:
            if(terminator_token == 1){
                SIGNAL(descrittore_semaforo, O_SEM);
                exit(0);
            }
            // Il file non è terminato:
            // Il buffer è stato scansionato tutto:
            SIGNAL(descrittore_semaforo, O_SEM);
        }
    }
    
    // CODICE FIGLIO O:
    pid_t o_pid = fork();
    if(o_pid == -1){ closing_function("fork", descrittore_memoria, descrittore_semaforo); exit(1); }
    if(o_pid == 0){
        int terminator_token = 0;
        
        while(1){
            WAIT(descrittore_semaforo, O_SEM);
            // Scansionamento Buffer:
            for(int i=0; i<N; i++){
                // Controlliamo che non sia un terminatore ed eventualmente stampiamo:
                if(pointer[i].tipo != -1) printf("%d \n",pointer[i].numero);
                else terminator_token = 1;
            }
            // Il file è terminato:
            if(terminator_token == 1) exit(0);
            
            // Il file non è terminato:
            // Azzeramento buffer:
            for(int i=0; i<N; i++){
                pointer[i].numero = 0;
                pointer[i].tipo = 0;
            }
            SIGNAL(descrittore_semaforo, P_SEM);
        }
    }
    
    // CODICE PADRE P:
    // Apertura File:
    FILE *file_stream = fopen(file_pathname,"r");
    if(file_stream == NULL){
        // Chiusura Figli:
        kill(m_pid, 9);
        kill(o_pid, 9);
        closing_function("fopen", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    // Variabili di supporto:
    char buffer_padre[KB];
    
    while(1){
        WAIT(descrittore_semaforo, P_SEM);
        // Riempiamo il buffer:
        for(int i=0; i<N; i++){
            // Il file è terminato:
            if(fgets(buffer_padre, KB, file_stream) == NULL){
                for(int j=i; j<N; j++){
                    pointer[j].numero = -1;
                    pointer[i].tipo = -1;
                }
                // Il buffer è pieno, segnaliamolo ad M:
                SIGNAL(descrittore_semaforo, M_SEM);
                // Attendo che i figli terminino:
                wait(NULL);
                wait(NULL);
                
                closing_function(NULL, descrittore_memoria, descrittore_semaforo);
                exit(0);
            }
            // Il file non è ancora terminato:
            if(buffer_padre[strlen(buffer_padre)-1] == '\n') buffer_padre[strlen(buffer_padre)-1] = '\0';
            
            pointer[i].numero = atoi(buffer_padre);
            if(pointer[i].numero < 0){
                printf("\nAttenzione, il file non può contenere numeri negativi.\n");
                pointer[i].numero = 0;
            }
                
            pointer[i].tipo = 0;
        }
        // Il buffer è pieno, segnaliamolo ad M:
        SIGNAL(descrittore_semaforo, M_SEM);
    }
}
