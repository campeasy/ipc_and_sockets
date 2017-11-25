// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define KB1 1024
#define KB2 2048

int substring_in_string(char *riga,char *parola){
    if(strstr(riga,parola) != NULL) return 0;
    else return -1;
}

typedef struct{
    long type;
    char messaggio[KB2];
} pacchetto;

int main(int argc,char *argv[]){
    // Controlli Preliminari:
    if(argc < 3){ printf("another-grep <parola> <pathname_file>\n"); exit(1); }
    char *file_pathname = argv[2];
    char *parola = argv[1];
    // Creazione PIPE:
    int my_pipe[2];
    if(pipe(my_pipe) == -1){ perror("pipe"); exit(1); }
    // Creazione Coda Messaggi:
    int descrittore_coda =  msgget(IPC_PRIVATE,IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_coda == -1){ perror("msgget"); exit(1); }
    
    // CODICE FIGLIO R:
    pid_t rpid = fork();
    if(rpid == -1){ perror("fork"); exit(1); }
    if(rpid == 0){
        // LETTURA DA FILE / SCRITTURA SU PIPE:
        FILE *file_stream; ;
        char buffer_lettura[KB2];
        if((file_stream = fopen(file_pathname,"r")) == NULL){ perror("fopen"); exit(1); }
        close(my_pipe[0]);  //Chiusura canale lettura PIPE.

        while(fgets(buffer_lettura, KB2, file_stream) != NULL){
            if(write(my_pipe[1], buffer_lettura, KB2) == -1){ perror("write"); exit(1); }
        }
        // Inserimento riga terminazione:
        write(my_pipe[1],"END",KB2);
        
        fclose(file_stream);
        exit(0);
    }
    
    // CODICE FIGLIO W:
    pid_t wpid = fork();
    if(wpid == -1){ perror("fork"); exit(1); }
    if(wpid == 0){
        // RICEZIONE DA CODA / STAMPA SU SCHERMO:
        pacchetto newpacket;
        newpacket.type = 1;
        
        while(1){
            // La chiamata NON utilizza IPC_NOWAIT, attende fino a quando vi è un messaggio nella coda:
            msgrcv(descrittore_coda, &newpacket, sizeof(pacchetto)-sizeof(long), 0, 0);
            if(strcmp(newpacket.messaggio,"END") == 0){
                if(msgctl(descrittore_coda,IPC_RMID,NULL) == -1){ perror("msgctl"); exit(1); }
                close(my_pipe[0]); close(my_pipe[1]);
                exit(0);
            }
            else printf("%s",newpacket.messaggio);
        }
        exit(0);
    }
    
    // Codice PADRE P:
    char buffer_dapipe[KB2];
    pacchetto mypacket;
    mypacket.type = 1;
    close(my_pipe[1]);  //Chiusura canale scrittura PIPE.
    
    while(1){
        read(my_pipe[0],buffer_dapipe,KB2);
        strcpy(mypacket.messaggio,buffer_dapipe);
        
        if(substring_in_string(buffer_dapipe,parola) == 0){
            if(msgsnd(descrittore_coda, &mypacket, sizeof(pacchetto)-sizeof(long), IPC_NOWAIT) == -1){ perror("msgsnd"); exit(1); }
        }
        
        if(strcmp(buffer_dapipe,"END") == 0){
            // La chiamata NON utilizza IPC_NOWAIT, attende fino a quando è possibile inviare il messaggio:
            msgsnd(descrittore_coda, &mypacket, sizeof(pacchetto)-sizeof(long), 0);
            // Attendiamo che il processo figlio completi il suo lavoro:
            waitpid(wpid,NULL,0);
            // Il lavoro del padre è terminato:
            exit(0);
        }
    }
}
