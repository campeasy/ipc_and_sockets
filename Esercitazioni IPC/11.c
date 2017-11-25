// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define KB1 1024
#define KB3 3072

#define ERROR -17

typedef struct{
    long type;
    char file[KB1];
    char testo[KB1];
} pacchetto;

int filter_function(char *buffer_testo, char *parola, int flag_inverted, int flag_insensitive){
    // Caso Inverted:
    if(flag_inverted == 1){
        // Sottocaso Insensitive:
        if(flag_insensitive == 1){
            if(strcasestr(buffer_testo,parola) != NULL) return -1;
            else return 1;
        }
        // Sottocaso Sensitive:
        else if(flag_insensitive == -1){
            if(strstr(buffer_testo,parola) != NULL) return -1;
            else return 1;
        }
    }
    // Caso Normale:
    else if(flag_inverted == -1){
        // Sottocaso Insensitive:
        if(flag_insensitive == 1){
            if(strcasestr(buffer_testo,parola) != NULL) return 1;
            else return -1;
        }
        // Sottocaso Sensitive:
        else if(flag_insensitive == -1){
            if(strstr(buffer_testo,parola) != NULL) return 1;
            else return -1;
        }
    }
    return ERROR;
}

int main(int argc, char *argv[]){
    // CONTROLLI SULL'INVOCAZIONE DEL PROGRAMMA:
    
    // Controllo preliminare, passaggio di parametri minimo:
    if(argc < 3){
        printf("\nErrore, rispettare la sintassi: \n");
        printf("myfgrep [-v] [-i] <parola> <pathname_file_1> [pathname_file_2] .. [pathname_file_n] \n\n");
        exit(1);
    }
    
    int token_inverted = -1;
    int token_insensitive = -1;
    int index = 0;
    char *parola;
    
    // Controlli sulle opzioni attive:
    if(strcmp(argv[1],"-v") == 0) token_inverted = 1;
    else if(strcmp(argv[1],"-i") == 0) token_insensitive = 1;
    
    if(strcmp(argv[2],"-i") == 0) token_insensitive = 1;
    else if(strcmp(argv[2],"-v") == 0) token_inverted = 1;
    
    // In base alle opzioni attive viene bufferizzata la parola e settato un flag sull'indice:
    if(token_insensitive == 1 && token_inverted == 1){ parola = argv[3]; index = 4; }
    else if(token_insensitive == 1 || token_inverted == 1){ parola = argv[2]; index = 3; }
    else if(token_insensitive == -1 && token_inverted == -1){ parola = argv[1]; index = 2; }
    
    // Controlli sui parametri passati:
    if((token_insensitive == 1 && token_inverted == 1) && (argc < 5)){
        printf("\nErrore, rispettare la sintassi: \n");
        printf("myfgrep [-v] [-i] <parola> <pathname_file_1> [pathname_file_2] .. [pathname_file_n] \n\n");
        exit(1);
    }
    else if((token_insensitive == 1 || token_inverted == 1) && (argc < 4)){
        printf("\nErrore, rispettare la sintassi: \n");
        printf("myfgrep [-v] [-i] <parola> <pathname_file_1> [pathname_file_2] .. [pathname_file_n] \n\n");
        exit(1);
    }
    // NOTE: controllo su argc < 3 non necessario poichè già effettuato.
    
    
    // CODICE DEL PROGRAMMA:
    
    // Creazione Coda Messaggi:
    int descrittore_coda = msgget(IPC_PRIVATE, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_coda == -1){ perror("msgget"); exit(1); }
    // Creazione PIPE:
    int my_pipe[2];
    pipe(my_pipe);
    
    // CODICE FIGLIO FILTER:
    pid_t filter_pid = fork();
    if(filter_pid == -1){ perror("fork"); exit(1); }
    if(filter_pid == 0){
        sleep(3);
        pacchetto received;
        received.type = 1;
        close(my_pipe[0]);
        
        while(1){
            // Chiamate BLOCCANTE (sospende il processo se non c'è nessun messaggio disponibile):
            msgrcv(descrittore_coda, &received, sizeof(pacchetto)-sizeof(long), 0, 0);
            
            char *buffer_file = received.file;
            char *buffer_testo = received.testo;
            
            // Stringa terminante ricevuta:
            if(strcmp(buffer_testo,"END") == 0){
                if(write(my_pipe[1], "END", KB3) == -1){ perror("write"); exit(1); }
                exit(0);
            }
            
            // Funzione che filtra:
            if(filter_function(buffer_testo,parola,token_inverted,token_insensitive) == 1){
                char tosend[KB3];
                strncat(tosend,buffer_file,KB3);
                strncat(tosend," : ",KB3);
                strncat(tosend,buffer_testo,KB3);
                
                if(write(my_pipe[1], tosend, KB3) == -1){ perror("write"); exit(1); }
            }
        }
    }
    
    // CREAZIONE FIGLI READER:
    pid_t reader_i_pid;
    pid_t aux_pid;
    for(int i=index; i<argc; i++){
        if(i > index){ aux_pid = reader_i_pid; }
        reader_i_pid = fork();
        
        if(reader_i_pid == -1){ perror("fork"); exit(1); }
        if(reader_i_pid == 0){
            if(i > index){ waitpid(aux_pid,NULL,0); }
            sleep(3);
            
            pacchetto tosend;
            tosend.type = 1;
            strncpy(tosend.file,argv[i],KB1);
            
            char buffer_lettura[KB1];
            
            FILE *file_stream;
            if((file_stream = fopen(argv[i],"r")) == NULL){ perror("fopen"); exit(1); }
            
            while(fgets(buffer_lettura, KB1, file_stream) != NULL){
                strncpy(tosend.testo,buffer_lettura,KB1);
                // Chiamate BLOCCANTE (sospende il processo finchè la coda di messaggi risulta piena):
                msgsnd(descrittore_coda, &tosend, sizeof(pacchetto)-sizeof(long), 0);
            }
            
            // Inserimento riga terminazione (solo se questo è l'ultimo processo):
            if((i+1) == argc){
                pacchetto terminatore;
                terminatore.type = 1;
                strncpy(terminatore.file,argv[i],KB1);
                strncpy(terminatore.testo,"END",KB1);
                // Chiamate BLOCCANTE (sospende il processo finchè la coda di messaggi risulta piena):
                msgsnd(descrittore_coda, &terminatore, sizeof(pacchetto)-sizeof(long), 0);
            }
            
            fclose(file_stream);
            exit(0);
        }
    }
    
    // Codice PADRE P:
    close(my_pipe[1]);
    char buffer_dapipe[KB3];
    
    while(1){
        read(my_pipe[0],buffer_dapipe,KB3);
        
        if(strcmp(buffer_dapipe,"END") == 0){
            if(msgctl(descrittore_coda, IPC_RMID, NULL) == -1){ printf("Errore, impossibile rimuovere coda. \n"); perror("msgctl"); exit(1); }
            close(my_pipe[0]);
            printf("\nEsecuzione terminata correttamente.\n");
            
            exit(0);
        }
        else printf("%s",buffer_dapipe);
    }
}
