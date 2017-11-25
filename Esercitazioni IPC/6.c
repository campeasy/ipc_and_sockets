// Developed by Salvatore Campisi

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/mman.h>

#define KB 128

// Procedura che verifica se una stringa è palindroma o meno:
int palindrome(char *stringa){
    int dim = strlen(stringa);
    int i = 0; int j = dim-1;
    for(; i<j && j>i; i++,j--) if(stringa[i] != stringa[j]) return -1;
    return 1;
}

int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Errore, rispettare la sintassi corretta: \n");
        printf("palindrome-filter pathname_file \n");
        exit(1);
    }
    
    // Creazione Pipes:
    int rp_pipe[2];
    int pw_pipe[2];
    pipe(rp_pipe);
    pipe(pw_pipe);
    
    // CODICE FIGLIO R:
    pid_t reader_pid = fork();
    if(reader_pid == -1){ perror("fork"); exit(1); }
    if(reader_pid == 0){
        // Apertura File da Mappare:
        int descrittore_file = open(argv[1], O_RDWR);
        if(descrittore_file == -1){ perror("open"); exit(1); }
        // Raccolta Informazioni su File:
        struct stat info;
        fstat(descrittore_file, &info);
        // Mappatura File in Memoria;
        char *pointer = NULL;
        pointer = (char *) mmap(NULL, info.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, descrittore_file, 0);
        if(pointer == NULL){ perror("mmap"); exit(1); }
        // Chiusura canale lettura Pipe rp_pipe:
        close(rp_pipe[0]);
        
        char tosend[KB];
        int i=0, flag_start = 0;
        for(; i<info.st_size; i++){
            if(pointer[i] == '\n'){
                // RESET BUFFER:
                strncpy(tosend,"",strlen(tosend));
                // COPIA SOTTOSTRINGA SUL BUFFER:
                strncpy(tosend,&pointer[flag_start],i-flag_start);
                // INVIO STRINGA SU PIPE:
                if(write(rp_pipe[1], tosend, KB) == -1){ perror("write"); exit(1); }
                if(i+1 < info.st_size){ flag_start = i+1; }
            }
        }
        strncpy(tosend,"END",KB);
        if(write(rp_pipe[1], tosend, KB) == -1){ perror("write"); exit(1); }
        
        exit(0);
    }
    
    // CODICE FIGLIO W:
    pid_t writer_pid = fork();
    if(writer_pid == -1){ perror("fork"); exit(1); }
    if(writer_pid == 0){
        close(pw_pipe[1]);
        char buffer_lettura[KB];
        while(1){
            if(read(pw_pipe[0], buffer_lettura, KB) == -1){ perror("read"); exit(1); }
        
            if(strcmp(buffer_lettura,"END") == 0){ exit(0); }
            else printf("%s \n",buffer_lettura);
        }
    }
    
    // CODICE PADRE P:
    close(rp_pipe[1]);
    close(pw_pipe[0]);
    
    char buffer_padre[KB];
    
    while(1){
        if(read(rp_pipe[0], buffer_padre, KB) == -1){ perror("read"); exit(1); }
        if(strcmp(buffer_padre,"END") == 0){
            if(write(pw_pipe[1], buffer_padre, KB) == -1){ perror("write"); exit(1); }
            exit(0);
        }
        else if(palindrome(buffer_padre) == 1){
            
            if(write(pw_pipe[1], buffer_padre, KB) == -1){ perror("write"); exit(1); }
        }
    }
}
