// ------------------------------------------------------------
// Salvatore Campisi
// C PROGRAMMING
// Anno Accademico 2017/2018
// Argomento: Unix Programming - IPC: Memoria Condivisa
// ------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define KB 1024

int main(){
    int descrittore_memoria;
    key_t chiave_memoria = IPC_PRIVATE;
    
    descrittore_memoria = shmget(chiave_memoria, KB, IPC_CREAT | IPC_EXCL | 0770);
    if(descrittore_memoria == -1){ perror("shmget"); exit(1); }
    
    pid_t firstpid = fork();
    if(firstpid == -1){ perror("fork"); exit(1); }
    if(firstpid == 0){
        // Nota bene: il casting può esserre effettuato al tipo di dato che vogliamo inserire in memoria.
        int *pointer = (int *) shmat(descrittore_memoria, NULL, 0);
        if(pointer == (int *) -1){ perror("shmat"); exit(1); }
        // Nota bene: è buona praticare inserire alla prima locazione mappata, il numero di locazioni realmente utilizzate.
        pointer[0] = 5;
        for(int i=1; i<=5; i++){
            pointer[i] = i;
        }
        
        printf("\nScrittura memoria condivisa... \n");
        exit(0);
    }
    
    pid_t secondpid = fork();
    if(secondpid == -1){ perror("fork"); exit(1); }
    if(secondpid == 0){
        waitpid(firstpid, NULL, 0);
        int *aux = (int *) shmat(descrittore_memoria, NULL, 0);
        if(aux == (int *) -1){ perror("shmat"); exit(1); }
        
        printf("Lettura memoria condivisa... \n");
        
        for(int i=1; i<=aux[0]; i++){
            printf("%d ",aux[i]);
        }
        printf("\n");
        
        exit(0);
    }
    
    waitpid(firstpid, NULL, 0);
    waitpid(secondpid, NULL, 0);
    shmctl(descrittore_memoria, IPC_RMID, NULL);
    
    printf("Terminazione in corso... \n\n");
    exit(0);
}
