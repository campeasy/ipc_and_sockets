// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>

#define DIM_USLEEP  2500
#define DIM_COMMAND 16
#define DIM_DEFAULT 256
#define KB          1024
#define TYPE_P      32767

typedef struct{
    long    type;
    char    command[DIM_COMMAND];
    char    file[DIM_DEFAULT];
    char    stringa[DIM_DEFAULT];
    // -------------------------
    int     k;
    char    text[DIM_DEFAULT];
} msg;

int WAIT(int sem_des, int sem_pos){
    struct sembuf wait_ops[1] = {{sem_pos,-1,0}};
    return semop(sem_des, wait_ops, 1);
}
int SIGNAL(int sem_des, int sem_pos){
    struct sembuf signal_ops[1] = {{sem_pos,+1,0}};
    return semop(sem_des, signal_ops, 1);
}

int search_function(char *pathname_file, char *word){
    FILE *file_stream = fopen(pathname_file,"r");
    if(file_stream == NULL) return -1;
    
    int cont = 0;
    char buffer_reader[KB];
    
    while(fgets(buffer_reader, KB, file_stream) != NULL){
        if(strcasestr(buffer_reader,word) != NULL) cont++;
    }
    return cont;
}

void setmsg(msg *message, long _type, char *_command, char *_file, char *_stringa, int _k, char *_text){
    message->type = _type;
    strncpy(message->command,_command,DIM_COMMAND);
    strncpy(message->file,_file,DIM_DEFAULT);
    strncpy(message->stringa,_stringa,DIM_DEFAULT);
    // -------------------------
    message->k = _k;
    strncpy(message->text,_text,DIM_DEFAULT);
    return;
}

void closing_function(char * error, int descrittore_coda, int descrittore_semaforo){
    if(error != NULL) perror(error);
    if(descrittore_coda != -1) msgctl(descrittore_coda, IPC_RMID, NULL);
    if(descrittore_semaforo != -1) semctl(descrittore_semaforo, 0, IPC_RMID, 0);
    return;
}

int main(int argc, char *argv[]){
    if(argc < 2) exit(1);
    // Creazione Coda:
    int descrittore_coda = msgget(IPC_PRIVATE, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_coda == -1){
        closing_function("msgget", -1, -1);
        exit(1);
    }
    // Creazione Array Semafori:
    int descrittore_semaforo = semget(IPC_PRIVATE, argc-1, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_semaforo == -1){
        closing_function("semget", descrittore_coda, -1);
        exit(1);
    }
    // Inizializzazione Semaforo Padre:
    semctl(descrittore_semaforo, 0, SETVAL, 0);
    // Variabili di supporto:
    char *pathname_directory;
    pid_t array_pid[argc];
    // PER OGNI CARTELLA SPECIFICATA:
    for(int i=1; i<argc; i++){
        // Bufferizzazione Pathname Directory:
        pathname_directory = argv[i];
        // Inizializzazione Semaforo I:
        semctl(descrittore_semaforo, i, SETVAL, 0);
        // Memorizzazione Pid:
        array_pid[i] = fork();
        // CODICE FIGLIO N:
        if(array_pid[i] == 0){
            msg messaggio;
            while(1){
                WAIT(descrittore_semaforo,i);
                msgrcv(descrittore_coda, &messaggio, sizeof(msg)-sizeof(long), i, 0);
                // CASO 1 : COMANDO RICHIESTO 'LIST'
                if(strcmp(messaggio.command,"list") == 0){
                    DIR *directory_stream = opendir(pathname_directory);
                    if(directory_stream == NULL){
                        setmsg(&messaggio, TYPE_P, "", "", "", 0, "END");
                        msgsnd(descrittore_coda, &messaggio, sizeof(msg)-sizeof(long), 0);
                        SIGNAL(descrittore_semaforo,0);
                    }
                    else if(directory_stream != NULL){
                        struct dirent* actual;
                        while((actual = readdir(directory_stream)) != NULL){
                            setmsg(&messaggio, TYPE_P, "", "", "", 0, actual->d_name);
                            msgsnd(descrittore_coda, &messaggio, sizeof(msg)-sizeof(long), 0);
                            SIGNAL(descrittore_semaforo,0);
                            WAIT(descrittore_semaforo,i);
                        }
                        setmsg(&messaggio, TYPE_P, "", "", "", 0, "END");
                        msgsnd(descrittore_coda, &messaggio, sizeof(msg)-sizeof(long), 0);
                        SIGNAL(descrittore_semaforo,0);
                    }
                }
                // CASO 2 : COMANDO RICHIESTO 'SIZE'
                else if(strcmp(messaggio.command,"size") == 0){
                    char nuovo_pathname[KB];
                    strncpy(nuovo_pathname,"",KB);  // Azzeramento Buffer.
                    strncat(nuovo_pathname,pathname_directory,KB);
                    strncat(nuovo_pathname,"/",KB);
                    strncat(nuovo_pathname,messaggio.file,KB);
                    
                    struct stat info;
                    stat(nuovo_pathname, &info);
                    setmsg(&messaggio, TYPE_P, "", "", "", info.st_size, "");
                    msgsnd(descrittore_coda, &messaggio, sizeof(msg)-sizeof(long), 0);
                    SIGNAL(descrittore_semaforo,0);
                }
                // CASO 3 : COMANDO RICHIESTO 'SEARCH'
                else if(strcmp(messaggio.command,"search") == 0){
                    char nuovo_pathname[KB];
                    strncpy(nuovo_pathname,"",KB);  // Azzeramento Buffer.
                    strncat(nuovo_pathname,pathname_directory,KB);
                    strncat(nuovo_pathname,"/",KB);
                    strncat(nuovo_pathname,messaggio.file,KB);
                    
                    int righe_con_stringa = search_function(nuovo_pathname, messaggio.stringa);
                    setmsg(&messaggio, TYPE_P, "", "", "", righe_con_stringa, "");
                    msgsnd(descrittore_coda, &messaggio, sizeof(msg)-sizeof(long), 0);
                    SIGNAL(descrittore_semaforo,0);
                }
            }
        }
    }
    // CODICE PADRE P:
    msg richiesta;
    char buffer_padre[KB];
    char buffer_file[KB];
    char buffer_stringa[KB];
    int n;
    printf("\nFai la tua scelta: \n");
    printf("(1) LIST N \n");
    printf("(2) SIZE N NOME-FILE \n");
    printf("(3) SEARCH N NOME-FILE \n");
    printf("(4) EXIT  \n");
    printf("-------------------------\n");
    printf("Inserisci numero operazione : ");
    
    while(1){
        fgets(buffer_padre, KB, stdin);
        if(strcmp(buffer_padre,"1\n") == 0){
            printf("Inserisci numero cartella :  "); scanf("%d",&n);
            setmsg(&richiesta, n, "list", "", "", 0, "");
            msgsnd(descrittore_coda, &richiesta, sizeof(msg)-sizeof(long), 0);
            SIGNAL(descrittore_semaforo, n);
            while(1){
                WAIT(descrittore_semaforo, 0);
                msgrcv(descrittore_coda, &richiesta, sizeof(msg)-sizeof(long), TYPE_P, 0);
                if(strcmp(richiesta.text,"END") == 0) break;
                else printf("%s \n",richiesta.text);
                SIGNAL(descrittore_semaforo, n);
            }
            printf("-------------------------\n");
            printf("Inserisci numero operazione : ");
        }
        else if(strcmp(buffer_padre,"2\n") == 0){
            printf("Inserisci numero cartella :  "); scanf("%d",&n);
            printf("Insersci nome file : "); scanf("%s",buffer_file);
            
            setmsg(&richiesta, n, "size", buffer_file, "", 0, "");
            msgsnd(descrittore_coda, &richiesta, sizeof(msg)-sizeof(long), 0);
            SIGNAL(descrittore_semaforo, n);
            WAIT(descrittore_semaforo, 0);
            msgrcv(descrittore_coda, &richiesta, sizeof(msg)-sizeof(long), TYPE_P, 0);
            printf("\nDimensioni File: %d \n",richiesta.k);
            printf("-------------------------\n");
            printf("Inserisci numero operazione : ");
            
        }
        else if(strcmp(buffer_padre,"3\n") == 0){
            printf("Inserisci numero cartella :  "); scanf("%d",&n);
            printf("Insersci nome file : "); scanf("%s",buffer_file);
            printf("Insersci stringa : "); scanf("%s",buffer_stringa);
            
            setmsg(&richiesta, n, "search", buffer_file, buffer_stringa, 0, "");
            msgsnd(descrittore_coda, &richiesta, sizeof(msg)-sizeof(long), 0);
            SIGNAL(descrittore_semaforo, n);
            WAIT(descrittore_semaforo, 0);
            msgrcv(descrittore_coda, &richiesta, sizeof(msg)-sizeof(long), TYPE_P, 0);
            printf("\nNumero di righe nel file contenenti la stringa indicata: %d \n",richiesta.k);
            printf("-------------------------\n");
            printf("Inserisci numero operazione : ");
        }
        else if(strcmp(buffer_padre,"4\n") == 0){
            printf("\nChiusura in corso...\n");
            for(int i=1; i<argc; i++) kill(array_pid[i],9);
            closing_function(NULL, descrittore_coda, descrittore_semaforo);
            sleep(1);
            printf("Chiusura terminata...\n");
            exit(0);
        }
        else if(strcmp(buffer_padre,"\n") != 0){
            printf("-------------------------\n");
            printf("Inserisci numero operazione : ");
        }
    }
}
