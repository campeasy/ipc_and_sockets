// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SEM_P 0
#define SEM_S 1
#define SEM_C 2

#define SEMAPHORES_NUM     3
#define COMPARE_BOX_NUM    1
#define MAX_WORLD_LEN      50   // BYTE

typedef struct{
    char    stringa_1[MAX_WORLD_LEN];
    char    stringa_2[MAX_WORLD_LEN];
    int     k;
} compare_box;

int WAIT(int sem_des, int sem_pos){
    struct sembuf wait_ops[1] = {{sem_pos,-1,0}};
    return semop(sem_des, wait_ops, 1);
}
int SIGNAL(int sem_des, int sem_pos){
    struct sembuf signal_ops[1] = {{sem_pos,+1,0}};
    return semop(sem_des, signal_ops, 1);
}

void closing_function(char *error, int descrittore_memoria, int descrittore_semaforo){
    if(error != NULL) perror(error);
    if(descrittore_memoria != -1) shmctl(descrittore_memoria, IPC_RMID, NULL);
    if(descrittore_semaforo != -1) semctl(descrittore_semaforo, 0, IPC_RMID, 0);
    return;
}

int main(int argc, char *argv[]){
    if(argc != 2) exit(1);
    // Creazione Memoria Condivisa:
    int descrittore_memoria = shmget(IPC_PRIVATE, (sizeof(compare_box) * COMPARE_BOX_NUM), IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_memoria == -1){
        closing_function("shmget", -1, -1);
        exit(1);
    }
    // Annessione Memoria Condivisa:
    compare_box *sh_memory = (compare_box *) shmat(descrittore_memoria, NULL, 0);
    // Creazione Array Semafori:
    int descrittore_semaforo = semget(IPC_PRIVATE, SEMAPHORES_NUM, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_semaforo == -1){
        closing_function("semget", descrittore_memoria, -1);
        exit(1);
    }
    // Inizializzazione Semafori:
    semctl(descrittore_semaforo, SEM_P, SETVAL, 0);
    semctl(descrittore_semaforo, SEM_S, SETVAL, 0);
    semctl(descrittore_semaforo, SEM_C, SETVAL, 0);
    // Apertura File:
    FILE *file_stream = fopen(argv[1],"r");
    if(file_stream == NULL){
        closing_function("fopen", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    
    // CODICE PROCESSO COMPARER:
    pid_t comparer_pid = fork();
    if(comparer_pid == -1){
        closing_function("fork", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    if(comparer_pid == 0){
        while(1){
            WAIT(descrittore_semaforo, SEM_C);
            if(strcasecmp(sh_memory->stringa_1, sh_memory->stringa_2) >= 0) sh_memory->k = 1;
            else sh_memory->k = -1;
            SIGNAL(descrittore_semaforo, SEM_S);
        }
    }
    
    // CODICE PROCESSO SORTER:
    pid_t sorter_pid = fork();
    if(sorter_pid == -1){
        closing_function("fork", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    if(sorter_pid == 0){
        // Variabili di Supporto:
        int dim = 0;
        char buffer_supporto[MAX_WORLD_LEN];
        // Determinazione Numero Righe:
        while(fgets(buffer_supporto, MAX_WORLD_LEN, file_stream) != NULL) dim++;
        // Riavvolgimento Stream:
        fseek(file_stream, 0, 0);
        // Array da ordinare:
        char *array[dim];
        
        for(int i=0; i<dim; i++){
            array[i] = malloc(MAX_WORLD_LEN);
            fgets(array[i], MAX_WORLD_LEN, file_stream);
        }
        // Ordinamento Array:
        for(int i=0; i<dim; i++){
            for(int j=i+1; j<dim; j++){
                strncpy(sh_memory->stringa_1, array[i], MAX_WORLD_LEN);
                strncpy(sh_memory->stringa_2, array[j], MAX_WORLD_LEN);
                
                // Confronto affidato al processo COMPARER:
                SIGNAL(descrittore_semaforo, SEM_C);
                WAIT(descrittore_semaforo, SEM_S);
                
                if(sh_memory->k == 1){
                    strncpy(buffer_supporto, array[j], MAX_WORLD_LEN);
                    strncpy(array[j], array[i], MAX_WORLD_LEN);
                    strncpy(array[i], buffer_supporto, MAX_WORLD_LEN);
                }
            }
        }
        kill(comparer_pid,9);
        // Invio contenuto array al padre per stampa:
        for(int i=0; i<dim; i++){
            if(i+1 == dim){
                strncpy(sh_memory->stringa_1, array[i], MAX_WORLD_LEN);
                strncpy(sh_memory->stringa_2, "END", MAX_WORLD_LEN);
                SIGNAL(descrittore_semaforo, SEM_P);
                
                exit(0);
            }
            strncpy(sh_memory->stringa_1, array[i], MAX_WORLD_LEN);
            strncpy(sh_memory->stringa_2, "NOT_END", MAX_WORLD_LEN);
            SIGNAL(descrittore_semaforo, SEM_P);
            WAIT(descrittore_semaforo, SEM_S);
        }
    }
    
    // CODICE PROCESSO PADRE:
    while(1){
        WAIT(descrittore_semaforo, SEM_P);
        if(strcmp(sh_memory->stringa_2, "END") == 0){
            printf("%s",sh_memory->stringa_1);
            printf("\n");
            
            closing_function(NULL, descrittore_memoria, descrittore_semaforo);
            exit(0);
        }
        else if(strcmp(sh_memory->stringa_2, "NOT_END") == 0) printf("%s",sh_memory->stringa_1);
        SIGNAL(descrittore_semaforo, SEM_S);
    }
}
