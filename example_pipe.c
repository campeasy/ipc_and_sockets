// ------------------------------------------------------------
// Salvatore Campisi
// C PROGRAMMING
// Anno Accademico 2017/2018
// Argomento: Unix Programming - IPC: Pipe
// ------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    int fildes[2];
    if(pipe(fildes) == -1) exit(1);
    
    pid_t firstpid = fork();
    if(firstpid > 0){
        // Parent Process:
        printf("Parent Process: \n");
        // Chiudiamo il canale di lettura, il genitore si occupa della scrittura.
        close(fildes[0]);
        // Scriviamo sul canale di scrittura della Pipe:
        write(fildes[1],"Hello there!",12);
    }
    else if(firstpid == 0){
        // Child Process:
        printf("Child Process: \n");
        // Chiudiamo il canale di scrittura, il figlio si occupa della lettura.
        close(fildes[1]);
        // Lettura dalla pipe:
        char buffer[12];
        read(fildes[0],buffer,12);
        printf("%s \n",buffer);
    }
    else if(firstpid == -1) exit(1);
}
