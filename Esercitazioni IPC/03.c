// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SEM_P 0
#define SEM_F 1

int WAIT(int sem_des, int sem_pos){
    struct sembuf wait_ops[1] = {{sem_pos,-1,0}};
    return semop(sem_des, wait_ops, 1);
}
int SIGNAL(int sem_des, int sem_pos){
    struct sembuf signal_ops[1] = {{sem_pos,+1,0}};
    return semop(sem_des, signal_ops, 1);
}

void closing_function(char *error, int sem_des, int shm_des){
    if(error != NULL) perror(error);
    semctl(sem_des, 0, IPC_RMID, 0);
    shmctl(shm_des, IPC_RMID, NULL);
    return;
}

int main(int argc, char *argv[]){
    if(argc < 2) exit(1);
    // Creazione Semafori:
    int descrittore_semaforo = semget(IPC_PRIVATE, 2, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_semaforo == -1){
        closing_function("semget", -1, -1);
        exit(1);
    }
    if(semctl(descrittore_semaforo, SEM_P, SETVAL, 0) == -1){
        closing_function("semctl", descrittore_semaforo, -1);
        exit(1);
    }
    if(semctl(descrittore_semaforo, SEM_F, SETVAL, 1) == -1){
        closing_function("semctl", descrittore_semaforo, -1);
        exit(1);
    }
    // Creazione Memoria Condivisa:
    int descrittore_memoria = shmget(IPC_PRIVATE, 2, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_memoria == -1){
        closing_function("shmget", descrittore_semaforo, -1);
        exit(1);
    }
    
    // CODICE FIGLIO F:
    pid_t firstpid = fork();
    if(firstpid == -1){
        closing_function("fork", descrittore_semaforo, descrittore_memoria);
        exit(1);
    }
    if(firstpid == 0){
        // Apertura File:
        FILE *file_stream = fopen(argv[1],"r");
        if(file_stream == NULL){
            closing_function("fopen", descrittore_semaforo, descrittore_memoria);
            exit(1);
        }
        // Annessione Memoria Condivisa:
        char *f_pointer = (char *) shmat(descrittore_memoria, NULL, 0);
        if(f_pointer == NULL){
            closing_function("shmat", descrittore_semaforo, descrittore_memoria);
            exit(1);
        }
        // Scansione File e Scrittura su Memoria Condivisa:
        char c = fgetc(file_stream);
        while(c != EOF && c != -1){
            WAIT(descrittore_semaforo, SEM_F);
                f_pointer[1] = c;
            SIGNAL(descrittore_semaforo, SEM_P);
            c = fgetc(file_stream);
        }
        WAIT(descrittore_semaforo, SEM_F);
            f_pointer[0] = '1';
        SIGNAL(descrittore_semaforo, SEM_P);
        
        exit(0);
    }
    
    // CODICE PADRE P:
    // Annessione Memoria Condivisa:
    char *p_pointer = (char *) shmat(descrittore_memoria, NULL, 0);
    if(p_pointer == NULL){ perror("shmat"); exit(1); }
    
    int caratteri = 0, parole = 0, righe = 0;
    
    while(1){
        WAIT(descrittore_semaforo, SEM_P);
        
        if(p_pointer[0] == '1'){
            closing_function(NULL, descrittore_semaforo, descrittore_memoria);
            
            printf("%s : %d caratteri , %d parole , %d righe \n",argv[1],caratteri,parole,righe);
            exit(0);
        }
        
        if(p_pointer[1] == '\n')
            righe++;
        else if(p_pointer[1] == ' ' || p_pointer[1] == ',' || p_pointer[1] == ';' || p_pointer[1] == ':' || p_pointer[1] == '.')
            parole++;
        else caratteri++;
        
        SIGNAL(descrittore_semaforo, SEM_F);
    }
}
