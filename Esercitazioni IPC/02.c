// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define ALPHABET_DIM 26

typedef struct{
    long type;
    int array[ALPHABET_DIM];
} pacchetto;

void closing_function(char *, int);
void init_array(int *,int);
int getpos(char);

int main(int argc, char *argv[]){
    if(argc < 2) exit(1);
    printf("\n");
    // Creazione Coda:
    int descrittore_coda = msgget(IPC_PRIVATE, IPC_CREAT|IPC_EXCL|0770);
    if(descrittore_coda == -1){
        closing_function("msgget", descrittore_coda);
        exit(1);
    }
    
    for(int i=1; i<argc; i++){
        char *file_pathname = argv[i];
        int numero_processo = i;
        
        // CODICE FIGLIO N:
        if(fork() == 0){
            sleep(i);
            // Apertura File:
            int descrittore_file = open(file_pathname, O_RDONLY);
            if(descrittore_file == -1){ perror("open"); exit(1); }
            // Raccolta Info File:
            struct stat info;
            if(fstat(descrittore_file, &info) == -1){ perror("fstat"); exit(1); }
            int dim_file = info.st_size;
            // Mappatura File in Memoria:
            char *pointer = mmap(NULL, dim_file, PROT_READ, MAP_PRIVATE, descrittore_file, 0);
            if(pointer == NULL){ perror("mmap"); exit(1); }
            
            // Creazione e inizializzazione pacchetto:
            pacchetto send;
            send.type = 1;
            init_array(send.array,ALPHABET_DIM);
            // Scansione File e Raccolta Statistica:
            int pos = -1;
            for(int i=0; i<dim_file; i++){
                pos = getpos(pointer[i]);
                if(pos != -1) send.array[pos] = send.array[pos] + 1;
            }
            // Stampa Statistica Locale:
            printf("Processo %d su file '%s' : \n",numero_processo,file_pathname);
            char aux = 'a';
            for(int i=0; i<ALPHABET_DIM; i++){
                printf("%c:%d ",aux,send.array[i]);
                aux++;
            }
            printf("\n");
            
            // Chiamata Bloccante, Invio statisca al padre:
            msgsnd(descrittore_coda, &send, sizeof(pacchetto)-sizeof(long), 0);
            
            exit(0);
        }
    }
    
    // CODICE PADRE P:
    pacchetto supp;
    int contatore_messaggi = 0;
    int statistica[ALPHABET_DIM];
    init_array(statistica,ALPHABET_DIM);
    
    // Ciclo per ricezione messaggi:
    while(1){
        // Chiamata Bloccante:
        msgrcv(descrittore_coda, &supp, sizeof(pacchetto)-sizeof(long), 1, 0);
        // Sommo la statistica locale del messaggio in quella globale:
        for(int i=0; i<ALPHABET_DIM; i++) statistica[i] = statistica[i] + supp.array[i];
        contatore_messaggi++;
        // Se questo era l'ultimo messaggio:
        if(contatore_messaggi == (argc-1)) break;
    }
    
    // Ricerca Massimo e Stampa Caratteristica Globale:
    printf("Processo Padre P: \n");
    // Variabili di supporto per la ricerca del massimo:
    int n_max = 0; char c_max; int flag_exequo = -1;
    
    char aux = 'a';
    for(int i=0; i<ALPHABET_DIM; i++){
        if(statistica[i] > n_max){
            n_max = statistica[i];
            c_max = aux;
        }
        else if(statistica[i] == n_max) flag_exequo = 1;
        
        printf("%c:%d ",aux,statistica[i]);
        aux++;
    }
    // Stampa Massimo:
    if(flag_exequo == -1) printf("\nLettera più utilizzata: %c \n",c_max);
    else printf("\nLettera più utilizzata: Ex-Aqueo \n");
    
    closing_function(NULL,descrittore_coda);
    printf("\n");
    exit(0);
}

void closing_function(char *error, int descrittore_coda){
    if(error != NULL) perror(error);
    if(descrittore_coda != -1) msgctl(descrittore_coda, IPC_RMID, NULL);
    return;
}

void init_array(int *array, int dim){
    for(int i=0; i<dim; i++) array[i] = 0;
    return;
}

int getpos(char c){
    // Controllo che sia un carattere:
    if(!(isalpha(c))) return -1;
    
    char smaller = 'a';
    char upper = 'A';
    // Controlla il carattere e ritorna la posizione corrispondente nell'alfabeto:
    for(int i=0; i<ALPHABET_DIM; i++){
        if(c == smaller || c == upper) return i;
        else{
            smaller++;
            upper++;
        }
    }
    return -1;
}
