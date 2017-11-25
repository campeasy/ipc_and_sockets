// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/mman.h>

#define KB 256

typedef struct{
    long    type;
    char    text[KB];
    int     contatore;
} pacchetto;

void closing_function(char *error, int descrittore_coda){
    if(error != NULL) perror(error);
    if(descrittore_coda != -1) msgctl(descrittore_coda, IPC_RMID, NULL);
    return;
}

void setpacchetto(pacchetto *temp, long _type, char *_text, int _contatore){
    temp->type = _type;
    strncpy(temp->text, _text, KB);
    temp->contatore = _contatore;
    return;
}

int istxt(char *file_pathname){
    if(strstr(file_pathname,".txt") != NULL) return 1;
    else return -1;
}

void recursive_scanning(DIR *directory_stream, char *directory_pathname, int descrittore_coda){
    // Variabili di supporto:
    char temp_pathname[KB];
    struct dirent *actual;
    pacchetto scanned;
    // Scansione dei file:
    while((actual = readdir(directory_stream)) != NULL){
        // Creazione del pathname per la voce attuale:
        strncpy(temp_pathname, "", KB);
        strncat(temp_pathname, directory_pathname, KB);
        strncat(temp_pathname, "/", KB);
        strncat(temp_pathname, actual->d_name, KB);
        
        struct stat info;
        stat(temp_pathname, &info);
        
        if(S_ISREG(info.st_mode)){
            if(istxt(temp_pathname) == 1){
                setpacchetto(&scanned, 2, temp_pathname, 0);
                msgsnd(descrittore_coda, &scanned, sizeof(pacchetto)-sizeof(long), 0);
                printf("Scanner : %s \n",temp_pathname);
                usleep(2500);
            }
        }
    }
    // Riavvolgimento Stream:
    rewinddir(directory_stream);
    // Scansione delle directory:
    while((actual = readdir(directory_stream)) != NULL){
        // Creazione del pathname per la voce attuale:
        strncpy(temp_pathname, "", KB);
        strncat(temp_pathname, directory_pathname, KB);
        strncat(temp_pathname, "/", KB);
        strncat(temp_pathname, actual->d_name, KB);
        
        struct stat info;
        stat(temp_pathname, &info);
        
        if(S_ISDIR(info.st_mode) && (strcmp(actual->d_name,".") != 0 && strcmp(actual->d_name,"..") != 0)){
            DIR *temp_directory_stream = opendir(temp_pathname);
            if(directory_stream != NULL){
                recursive_scanning(temp_directory_stream, temp_pathname, descrittore_coda);
                closedir(temp_directory_stream);
            }
        }
    }
    return;
}

int main(int argc, char *argv[]){
    if(argc != 2) exit(1);
    // Creazione Coda:
    int descrittore_coda = msgget(IPC_PRIVATE, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_coda == -1){
        closing_function("msgget", -1);
        exit(1);
    }
    
    // CODICE PROCESSO SCANNER:
    pid_t pid_scanner = fork();
    if(pid_scanner == -1){
        closing_function("fork", descrittore_coda);
        exit(1);
    }
    if(pid_scanner == 0){
        // Variabili di supporto:
        char *directory_pathname = argv[1];
        
        // Apertura Directory:
        DIR *directory_stream = opendir(directory_pathname);
        if(directory_stream == NULL){
            perror("opendir");
            pacchetto pacchetto_terminatore;
            setpacchetto(&pacchetto_terminatore, 2, "END", 0);
            msgsnd(descrittore_coda, &pacchetto_terminatore, sizeof(pacchetto)-sizeof(long), 0);
            usleep(2500);
            
            exit(1);
        }
        // Scanning Ricorsivo:
        recursive_scanning(directory_stream, directory_pathname, descrittore_coda);
        // Pacchetto Terminatore:
        pacchetto pacchetto_terminatore;
        setpacchetto(&pacchetto_terminatore, 2, "END", 0);
        msgsnd(descrittore_coda, &pacchetto_terminatore, sizeof(pacchetto)-sizeof(long), 0);
        usleep(2500);
        
        exit(0);
    }
    
    // CODICE PROCESSO ANALYZER:
    pid_t pid_analyzer = fork();
    if(pid_analyzer == -1){
        closing_function("fork", descrittore_coda);
        exit(1);
    }
    if(pid_analyzer == 0){
        // Variabili di supporto:
        pacchetto to_analyze;
        int descrittore_file;
        char *pointer = NULL;
        int cont = 0;
        
        while(1){
            cont = 0;
            msgrcv(descrittore_coda, &to_analyze, sizeof(pacchetto)-sizeof(long), 2, 0);
            // Controllo Pacchetto Terminatore:
            if(strcmp(to_analyze.text, "END") == 0){
                pacchetto pacchetto_terminatore;
                setpacchetto(&pacchetto_terminatore, 1, "END", 0);
                msgsnd(descrittore_coda, &pacchetto_terminatore, sizeof(pacchetto)-sizeof(long), 0);
                usleep(2500);
                
                exit(0);
            }
            // Apertura File:
            descrittore_file = open(to_analyze.text, O_RDONLY);
            if(descrittore_file != -1){
                // Raccolta Informazioni File:
                struct stat info;
                fstat(descrittore_file, &info);
                // Mappatura File in Memoria:
                pointer = (char *) mmap(NULL, info.st_size, PROT_READ, MAP_PRIVATE, descrittore_file, 0);
                for(int i=0; i<info.st_size; i++){
                    int c = (int) pointer[i];
                    if((c >= 65 || c <= 90) || (c >= 97 || c <= 122)) cont = cont + 1;
                }
                // Dismissione Mappatura:
                munmap(pointer, info.st_size);
                // Stampa Risultato ed invio risultati al padre:
                printf("Analyzer : %s , %d \n", to_analyze.text, cont);
                setpacchetto(&to_analyze, 1, "", cont);
                msgsnd(descrittore_coda, &to_analyze, sizeof(pacchetto)-sizeof(long), 0);
                usleep(2500);
            }
            else perror("open");
        }
    }
    
    // CODICE PROCESSO PADRE:
    pacchetto result;
    int totale = 0;
    
    while(1){
        msgrcv(descrittore_coda, &result, sizeof(pacchetto)-sizeof(long), 1, 0);
        // Controllo Pacchetto Terminatore:
        if(strcmp(result.text, "END") == 0){
            kill(pid_scanner,9);
            kill(pid_analyzer,9);
            closing_function(NULL, descrittore_coda);
            
            printf("Padre : totale conteggiato = %d \n",totale);
            
            exit(0);
        }
        // Incremento Totale:
        totale = totale + result.contatore;
    }
}
