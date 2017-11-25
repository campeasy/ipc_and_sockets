// ------------------------------------------------------------
// Salvatore Campisi
// C PROGRAMMING
// Anno Accademico 2017/2018
// Argomento: Unix Programming - IPC: FIFO
// ------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
    char *fifo_pathname = "/tmp/myfifo";
    if(mkfifo(fifo_pathname, 0777) == -1){ printf("Coda FIFO già esistente! \n"); exit(1); }
    int fd;
    
    pid_t firstpid = fork();
    if(firstpid > 0){
        // Parent Process:
        printf("Parent Process: \n");
        if((fd = open("/tmp/myfifo",O_WRONLY)) == -1){ printf("Errore apertura Parent! \n"); exit(1); }

        write(fd,"Hello there!",13);
        
        close(fd);
        exit(0);
    }
    else if(firstpid == 0){
        // Child Process:
        printf("Child Process: \n");

        char buffer[13];
        if((fd = open("/tmp/myfifo",O_RDONLY)) == -1){ printf("Errore apertura Child! \n"); exit(1); }
        
        read(fd,buffer,13);
        printf("%s \n",buffer);
        
        unlink(fifo_pathname);
        exit(0);
    }
    else if(firstpid == -1) exit(1);
}
