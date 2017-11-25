// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_DIM 28
#define KB 1024

void closing_function(char *error, int shm_des, int sem_des){
    if(error != NULL) perror(error);
    if(shm_des != -1) shmctl(shm_des, IPC_RMID, NULL);
    if(sem_des != -1) semctl(sem_des, 0, IPC_RMID, 0);
    return;
}
int WAIT(int sem_des, int sem_pos){
    struct sembuf wait_ops[1] = {{sem_pos,-1,0}};
    return semop(sem_des, wait_ops, 1);
}
int SIGNAL(int sem_des, int sem_pos){
    struct sembuf signal_ops[1] = {{sem_pos,+1,0}};
    return semop(sem_des, signal_ops, 1);
}
int getpos(char);

int main(int argc, char *argv[]){
    if(argc < 2) exit(1);
    // Creazione Memoria Condivisa:
    int descrittore_memoria = shmget(IPC_PRIVATE, SHM_DIM, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_memoria == -1){
        closing_function("shmget", -1, -1);
        exit(1);
    }
    // Annessione Memoria Condivisa:
    int *pointer = (int *) shmat(descrittore_memoria, NULL, 0);
    if(pointer == NULL){
        closing_function("shmat", descrittore_memoria, -1);
        exit(1);
    }
    // Inizializzazione Memoria Condivisa:
    for(int i=0; i<SHM_DIM; i++) pointer[i] = 0;
    
    // Creazione Semaforo:
    int descrittore_semaforo = semget(IPC_PRIVATE, 1, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_semaforo == -1){
        closing_function("semget", descrittore_memoria, -1);
        exit(1);
    }
    // Inizializzazione Semaforo:
    if(semctl(descrittore_semaforo, 0, SETVAL, 1) == -1){
        closing_function("semctl", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    
    char file_pathname[KB];
    for(int i=1; i<argc; i++){
        strncpy(file_pathname,argv[i],KB);
        
        // CODICE GENERICO FIGLIO N:
        pid_t generic_pid = fork();
        if(generic_pid == -1){ closing_function("fork", descrittore_memoria, descrittore_semaforo); exit(1); }
        if(generic_pid == 0){
            // Apertura File:
            int descrittore_file = open(file_pathname, O_RDONLY);
            if(descrittore_file == -1){
                WAIT(descrittore_semaforo, 0);
                pointer[27] = pointer[27] + 1;
                SIGNAL(descrittore_semaforo, 0);
                
                perror("open");
                exit(1);
            }
            // Ricavo Informazioni File:
            struct stat info; fstat(descrittore_file, &info);
            int file_dim = info.st_size;
            // Mappatura File in Memoria:
            char *file_pointer = (char *) mmap(NULL, file_dim, PROT_READ, MAP_PRIVATE, descrittore_file, 0);
            
            int pos;
            // Scansione File:
            for(int j=0; j<file_dim; j++){
                pos = getpos(file_pointer[j]);
                // Se il file è un carattere:
                if(pos != -1){
                    WAIT(descrittore_semaforo, 0);
                    pointer[pos] = pointer[pos] + 1;
                    SIGNAL(descrittore_semaforo, 0);
                }
            }
            // Incremento della dimensione totale e contatore file terminati:
            WAIT(descrittore_semaforo, 0);
            pointer[0] = pointer[0] + file_dim;
            pointer[27] = pointer[27] + 1;
            SIGNAL(descrittore_semaforo, 0);
            
            exit(0);
        }
    }
    
    // CODICE PADRE P:
    int cont = 0;
    // Busy Waiting: attendiamo che tutti i figli abbiano terminato il proprio lavoro:
    while(1){
        WAIT(descrittore_semaforo, 0);
        cont = pointer[27];
        SIGNAL(descrittore_semaforo, 0);
        
        if(cont == (argc-1)) break;
    }
    
    char c = 'a';
    int percentuale = 0;
    for(int i=1; i<27; i++){
        WAIT(descrittore_semaforo, 0);
        percentuale = (pointer[i] * 100)/pointer[0];
        SIGNAL(descrittore_semaforo, 0);
        
        printf("%c : %d%c   ",c,percentuale,37);
        if(i % 3 == 0) printf("\n");
        
        c++;
    }
    printf("\n");
    closing_function(NULL, descrittore_memoria, descrittore_semaforo);
    exit(0);
}

// Procedura che restituisce la posizione del carattere nella memoria condivisa:
int getpos(char c){
    if(c == 'a' || c == 'A')                    return 1;
    else if(c == 'b' || c == 'B')               return 2;
    else if(c == 'c' || c == 'C')               return 3;
    else if(c == 'd' || c == 'D')               return 4;
    else if(c == 'e' || c == 'E')               return 5;
    else if(c == 'f' || c == 'F')               return 6;
    else if(c == 'g' || c == 'G')               return 7;
    else if(c == 'h' || c == 'H')               return 8;
    else if(c == 'i' || c == 'I')               return 9;
    else if(c == 'j' || c == 'J')               return 10;
    else if(c == 'k' || c == 'K')               return 11;
    else if(c == 'l' || c == 'L')               return 12;
    else if(c == 'm' || c == 'M')               return 13;
    else if(c == 'n' || c == 'N')               return 14;
    else if(c == 'o' || c == 'O')               return 15;
    else if(c == 'p' || c == 'P')               return 16;
    else if(c == 'q' || c == 'Q')               return 17;
    else if(c == 'r' || c == 'R')               return 18;
    else if(c == 's' || c == 'S')               return 19;
    else if(c == 't' || c == 'T')               return 20;
    else if(c == 'u' || c == 'U')               return 21;
    else if(c == 'v' || c == 'V')               return 22;
    else if(c == 'w' || c == 'W')               return 23;
    else if(c == 'x' || c == 'X')               return 24;
    else if(c == 'y' || c == 'Y')               return 25;
    else if(c == 'z' || c == 'Z')               return 26;
    else return -1;
}
