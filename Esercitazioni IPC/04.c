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

// Dichiarazione Funzioni:
int filter(char *,char *);

int main(int argc, char *argv[]){
    if(argc < 3) exit(1);
    
    char *word = argv[1];
    char *pathname = argv[2];
    
    // Creazione PIPE:
    int my_pipe[2];
    pipe(my_pipe);
    // Creazione CODA:
    int descrittore_coda = msgget(IPC_PRIVATE, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_coda == -1){ perror("msgget"); exit(1); }
    
    // CODICE FIGLIO R:
    pid_t reader_pid = fork();
    if(reader_pid == -1){ perror("fork"); exit(1); }
    if(reader_pid == 0){
        FILE *file_stream = fopen(pathname, "r");
        if(file_stream == NULL){ perror("fopen"); exit(1); }
        
        close(my_pipe[0]);
        
        char buffer_reader[KB];
        while(fgets(buffer_reader, KB, file_stream) != NULL){
            if(write(my_pipe[1], buffer_reader, KB) == -1){ perror("write"); exit(1); }
        }
        
        // Stringa terminatore:
        if(write(my_pipe[1], "END", KB) == -1){ perror("write"); exit(1); }
        
        fclose(file_stream);
        exit(0);
    }
    
    // CODICE FIGLIO W:
    pid_t writer_pid = fork();
    if(writer_pid == -1){ perror("fork"); exit(1); }
    if(writer_pid == 0){
        pacchetto received;
        char buffer_writer[KB];
        while(1){
            // Chiamata Bloccante:
            msgrcv(descrittore_coda, &received, sizeof(pacchetto)-sizeof(long), 1, 0);
            strncpy(buffer_writer,received.text,KB);
            // Controllo Stringa terminatore:
            if(strcmp(buffer_writer,"END") == 0){
                exit(0);
            }
            
            printf("%s",buffer_writer);
        }
    }
    
    // CODICE PADRE P:
    char buffer_padre[KB];
    pacchetto tosend;
    tosend.type = 1;
    
    close(my_pipe[1]);
    
    while(1){
        if(read(my_pipe[0], buffer_padre, KB) == -1){ perror("read"); exit(1); }
        if(strcmp(buffer_padre,"END") == 0){
            strncpy(tosend.text,"END",KB);
            // Chiamata Bloccante:
            if(msgsnd(descrittore_coda, &tosend, sizeof(pacchetto)-sizeof(long), IPC_NOWAIT) == -1){ perror("msgsnd"); exit(1); }
            
            waitpid(writer_pid,NULL,0);
            if(msgctl(descrittore_coda, IPC_RMID, NULL) == -1){ perror("msgctl"); exit(1); }
            
            exit(0);
        }
        if(filter(buffer_padre,word) == 1){
            strncpy(tosend.text,buffer_padre,KB);
            // Chiamata Bloccante:
            if(msgsnd(descrittore_coda, &tosend, sizeof(pacchetto)-sizeof(long), IPC_NOWAIT) == -1){ perror("msgsnd"); exit(1); }
        }
    }
}

int filter(char *stringa, char *parola){
    if(strstr(stringa,parola) != NULL || strcasestr(stringa,parola) != NULL) return 1;
    else return -1;
}










