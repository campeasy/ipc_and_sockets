// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define KB       1024 // MAX_PATH_LEN
#define N_PAC    1    // DIMENSIONE SHM_MEMORY

#define SEM_NUM  4
#define SEM_PD   0    // NUMERO SEMAFORO PADRE
#define SEM_ST   1    // NUMERO SEMAFORO STARTER
#define SEM_SN   2    // NUMERO SEMAFORO GENERICO SCANNER
#define MUTEX    3

// Tipo della memoria condivisa:
typedef struct{
    int  k;
    char pathname[KB];
    char radice[KB];
} pacchetto;

// Operazioni di servizio sui semafori:
int WAIT(int descrittore_semaforo, int num_semaforo){
    struct sembuf op_wait[1] = {{num_semaforo, -1, 0}};
    return semop(descrittore_semaforo, op_wait, 1);
}
int SIGNAL(int descrittore_semaforo, int num_semaforo){
    struct sembuf op_signal[1] = {{num_semaforo, +1, 0}};
    return semop(descrittore_semaforo, op_signal, 1);
}

// Settaggio della memoria condivisa:
void setpacchetto(pacchetto *temp, int _k, char *_pathname, char *_radice){
    temp->k = _k;
    strncpy(temp->pathname, _pathname, KB);
    strncpy(temp->radice, _radice, KB);
    return;
}

// Funzione di chiusura per non lasciare risorse IPC occupate:
void closing_function(char *error, int descrittore_memoria, int descrittore_semaforo){
    if(error != NULL) perror(error);
    if(descrittore_memoria != -1) shmctl(descrittore_memoria, IPC_RMID, NULL);
    if(descrittore_semaforo != -1) semctl(descrittore_semaforo, 0, IPC_RMID, 0);
    return;
}

// Analisi Ricorsiva del File System:
void recursive_scanning(char *directory_pathname, char *directory_radice, int descrittore_semaforo, pacchetto *temp){
    DIR *directory_stream = opendir(directory_pathname);
    if(directory_stream == NULL){ perror("opendir"); return; }
    
    // Variabili di Supporto:
    char temp_pathname[KB];
    struct dirent *actual;
    struct stat info;
    
    // Invio di un pacchetto per ogni voce di directory che NON è una cartella:
    while((actual = readdir(directory_stream))  != NULL){
        strncpy(temp_pathname, "", KB);
        strncat(temp_pathname, directory_pathname, KB);
        strncat(temp_pathname, "/", KB);
        strncat(temp_pathname, actual->d_name, KB);
        
        stat(temp_pathname, &info);
        if(!S_ISDIR(info.st_mode)){
            WAIT(descrittore_semaforo, MUTEX);
            setpacchetto(temp, 0, temp_pathname, directory_radice);
            SIGNAL(descrittore_semaforo, MUTEX);
            
            SIGNAL(descrittore_semaforo, SEM_ST);
            WAIT(descrittore_semaforo, SEM_SN);
        }
    }
    // Riavvolgimento Stream:
    seekdir(directory_stream, 0);
    // Passo ricorsivo per ogni directory:
    while((actual = readdir(directory_stream)) != NULL){
        strncpy(temp_pathname, "", KB);
        strncat(temp_pathname, directory_pathname, KB);
        strncat(temp_pathname, "/", KB);
        strncat(temp_pathname, actual->d_name, KB);
        
        stat(temp_pathname, &info);
        if(S_ISDIR(info.st_mode)){
            if(strcmp(actual->d_name, ".") != 0 && strcmp(actual->d_name, "..") != 0){
                recursive_scanning(temp_pathname, directory_radice, descrittore_semaforo, temp);
            }
        }
    }
    return;
}

int main(int argc, char *argv[]){
    if(argc < 2) exit(1);
    // Creazione Memoria Condivisa:
    int descrittore_memoria = shmget(IPC_PRIVATE, (sizeof(pacchetto)*N_PAC), IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_memoria == -1){
        closing_function("shmget", -1, -1);
        exit(1);
    }
    // Annessione Memoria Condivisa:
    pacchetto *shm_pointer = (pacchetto *) shmat(descrittore_memoria, NULL, 0);
    if(shm_pointer == NULL){
        closing_function("shmat", descrittore_memoria, -1);
        exit(1);
    }
    // Inizializzazione Memoria Condivisa:
    setpacchetto(&shm_pointer[0], -1, "", "");
    
    // Creazione Array di Semafori:
    int descrittore_semaforo = semget(IPC_PRIVATE, SEM_NUM, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_semaforo == -1){
        closing_function("semget", descrittore_memoria, -1);
        exit(1);
    }
    // Inizializzazione Semafori:
    if(semctl(descrittore_semaforo, SEM_PD, SETVAL, 0) == -1){
        closing_function("semctl", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    if(semctl(descrittore_semaforo, SEM_ST, SETVAL, 0) == -1){
        closing_function("semctl", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    if(semctl(descrittore_semaforo, SEM_SN, SETVAL, 1) == -1){
        closing_function("semctl", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    if(semctl(descrittore_semaforo, MUTEX, SETVAL, 1) == -1){
        closing_function("semctl", descrittore_memoria, descrittore_semaforo);
        exit(1);
    }
    
    char directory_pathname[KB];
    for(int i=1; i<argc; i++){
        strncpy(directory_pathname, "", KB);
        strncpy(directory_pathname, argv[i], KB);
        // CODICE PROCESSO SCANNER N:
        if(fork() == 0){
            // Scanning Ricorsivo:
            recursive_scanning(directory_pathname, directory_pathname, descrittore_semaforo, &shm_pointer[0]);
            
            // Settaggio Pacchetto Terminatore:
            WAIT(descrittore_semaforo, MUTEX);
            setpacchetto(&shm_pointer[0], 0, "TERMINATED", directory_pathname);
            SIGNAL(descrittore_semaforo, MUTEX);
            
            SIGNAL(descrittore_semaforo, SEM_ST);
            
            exit(0);
        }
    }
    
    // CODICE PROCESSO STARTER:
    pid_t starter_pid = fork();
    if(starter_pid == 0){
        // Variabili di supporto:
        int contatore = 0;
        int buffer_int;
        char buffer_starter[KB];
        char buffer_radice[KB];
        struct stat info;

        while(1){
            WAIT(descrittore_semaforo, SEM_ST);
            
            WAIT(descrittore_semaforo, MUTEX);
            buffer_int = shm_pointer[0].k;
            strncpy(buffer_starter, shm_pointer[0].pathname, KB);
            strncpy(buffer_radice, shm_pointer[0].radice, KB);
            SIGNAL(descrittore_semaforo, MUTEX);
            
            // Abbiamo ricevuto un pacchetto terminatore:
            if(strcmp(buffer_starter, "TERMINATED") == 0){
                contatore++;
                // Ultimo Scanner ha terminato:
                if(contatore >= (argc-1)){
                    WAIT(descrittore_semaforo, MUTEX);
                    setpacchetto(&shm_pointer[0], 0, "ST_TERMINATED", "");
                    SIGNAL(descrittore_semaforo, MUTEX);
                    
                    SIGNAL(descrittore_semaforo, SEM_SN);
                    SIGNAL(descrittore_semaforo, SEM_PD);
                    
                    exit(0);
                }
                // Esistono ancora scanner attivi:
                SIGNAL(descrittore_semaforo, SEM_SN);
            }
            // Abbiamo ricevuto un pacchetto contenente un pathname:
            stat(buffer_starter, &info);
            WAIT(descrittore_semaforo, MUTEX);
            setpacchetto(&shm_pointer[0], info.st_blocks, "", buffer_radice);
            SIGNAL(descrittore_semaforo, MUTEX);
            
            SIGNAL(descrittore_semaforo, SEM_PD);
        }
    }
    
    // CODICE PROCESSO PADRE:
    pacchetto totale[(argc-1)];
    for(int j=0; j<argc-1; j++){
        totale[j].k = 0;
        strncpy(totale[j].radice, argv[j+1], KB);
        strncpy(totale[j].pathname, "" , KB);
    }
    int buffer_int;
    char buffer_padre[KB];
    char buffer_radice[KB];
    
    while(1){
        WAIT(descrittore_semaforo, SEM_PD);
        
        WAIT(descrittore_semaforo, MUTEX);
        buffer_int = shm_pointer[0].k;
        strncpy(buffer_padre, shm_pointer[0].pathname, KB);
        strncpy(buffer_radice, shm_pointer[0].radice, KB);
        SIGNAL(descrittore_semaforo, MUTEX);
        
        // Lo starter ha terminato il suo lavoro:
        if(strcmp(buffer_padre, "ST_TERMINATED") == 0){
            for(int j=0; j<argc-1; j++) printf("%d - %s \n", totale[j].k, totale[j].radice);
            
            closing_function(NULL, descrittore_memoria, descrittore_semaforo);
            exit(0);
        }
        // Lo starter non ha ancora terminato il suo lavoro:
        for(int j=0; j<argc-1; j++){
            if(strcmp(buffer_radice, totale[j].radice) == 0){
                totale[j].k += buffer_int;
                break;
            }
            
        }
        // Azzeramento Memoria Condivisa:
        WAIT(descrittore_semaforo, MUTEX);
        setpacchetto(&shm_pointer[0], 0, "", "");
        SIGNAL(descrittore_semaforo, MUTEX);
        
        SIGNAL(descrittore_semaforo, SEM_SN);
    }
}
