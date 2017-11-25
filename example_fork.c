// ------------------------------------------------------------
// Salvatore Campisi
// C PROGRAMMING
// Anno Accademico 2017/2018
// Argomento: Unix Programming - fork
// ------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    pid_t pid1;
    pid1 = fork();
    
    if(pid1 != 0 && pid1 != -1){
        waitpid(pid1,NULL,0);
        printf("PROCESSO PADRE: %d - PADRE: %d \n",getpid(),getppid());
        exit(0);
    }
    if(pid1 == 0){
        pid_t pid2;
        pid2 = fork();
        if(pid2 != 0 && pid2 != -1){
            waitpid(pid2,NULL,0);
            printf("PROCESSO FIGLIO: %d - PADRE: %d \n",getpid(),getppid());
            exit(0);
        }
        if(pid2 == 0){
            pid_t pid3;
            pid3 = fork();
            if(pid3 != 0 && pid3 != -1){
                waitpid(pid3,NULL,0);
                printf("PROCESSO NIPOTE: %d - PADRE: %d \n",getpid(),getppid());
                exit(0);
            }
            if(pid3 == 0){
                printf("PROCESSO PRONIPOTE: %d - PADRE: %d \n",getpid(),getppid());
                exit(0);
            }
            if(pid3 == -1) exit(1);
        }
        if(pid2 == -1) exit(1);
    }
    if(pid1 == -1) exit(1);
}
