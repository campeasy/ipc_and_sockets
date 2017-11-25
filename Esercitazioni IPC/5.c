// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define KB 128

typedef struct{
    long type;
    char text[KB];
} pacchetto;

int palindrome(char *stringa){
    int i=0, j=0;
    // Verifico se l'ultimo carattere è '\n' o meno:
    if(stringa[strlen(stringa)-1] == '\n') j = strlen(stringa)-2;
    else j = strlen(stringa)-1;
    for(; j>i; i++,j--){
        if(stringa[i] != stringa[j]) return -1; // Stringa non Palindroma
    }
    return 1; // Stringa Palindroma
}

void closing_function(char *error, int descrittore_coda){
    if(error != NULL) perror(error);
    if(descrittore_coda != -1) msgctl(descrittore_coda, IPC_RMID, NULL);
    return;
}

int main(int argc, char *argv[]){
    if(argc != 3) exit(1);
    // Creazione Coda:
    int descrittore_coda = msgget(IPC_PRIVATE, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_coda == -1){
        closing_function("msgget", -1);
        exit(1);
    }
    
    pid_t reader_pid = fork();
    if(reader_pid == -1){
        closing_function("fork", descrittore_coda);
        exit(1);
    }
    if(reader_pid == 0){
        // CODICE PROCESSO R:
        
        FILE* file_stream = fopen(argv[1],"r");
        if(file_stream == NULL){ perror("fopen"); exit(1); }
        
        pacchetto readed;
        readed.type = 6; // Tipo di messaggi per il Padre.
        
        while(fgets(readed.text, KB, file_stream) != NULL){
            // Chiamata Bloccante:
            msgsnd(descrittore_coda, &readed, sizeof(pacchetto)-sizeof(long), 0);
            usleep(1000);
        }
        // Pacchetto Terminatore:
        strncpy(readed.text, "END", KB);
        // Chiamata Bloccante per Pacchetto Terminatore:
        msgsnd(descrittore_coda, &readed, sizeof(pacchetto)-sizeof(long), 0);
        
        exit(0);
    }
    
    pid_t writer_pid = fork();
    if(writer_pid == -1){
        closing_function("fork", descrittore_coda);
        exit(1);
    }
    if(writer_pid == 0){
        // CODICE PROCESSO W:
        
        FILE* file_stream = fopen(argv[2],"w+");
        if(file_stream == NULL){ perror("fopen"); exit(1); }
        
        pacchetto towrite;
        
        while(1){
            // Chiamata Bloccante:
            msgrcv(descrittore_coda, &towrite, sizeof(pacchetto)-sizeof(long), 9, 0);
            // Controllo Stringa Terminatore:
            if(strcmp(towrite.text,"END") == 0) exit(0);
            
            fputs(towrite.text, file_stream);
        }
    }
    
    // CODICE PROCESSO P:
    pacchetto toscan;
    
    while(1){
        toscan.type = 6;
        msgrcv(descrittore_coda, &toscan, sizeof(pacchetto)-sizeof(long), 6, 0);
        
        if(strcmp(toscan.text,"END") == 0){
            toscan.type = 9; // Tipo di messaggi per il Writer
            msgsnd(descrittore_coda, &toscan, sizeof(pacchetto)-sizeof(long), 0);
            
            waitpid(writer_pid,NULL,0);
            closing_function(NULL, descrittore_coda);
            exit(0);
        }
        else if(palindrome(toscan.text) == 1){
            toscan.type = 9; // Tipo di messaggi per il Writer
            msgsnd(descrittore_coda, &toscan, sizeof(pacchetto)-sizeof(long), 0);
        }
    }
}
