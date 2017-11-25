// Developed by Salvatore Campisi

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/stat.h>

#define KB 512
#define ERROR -2

typedef struct{
    int i_flag;
    int v_flag;
    char *word;
    char *file_pathname;
} program_info;

// Dichiarazione Funzioni:
void display_error();
void fromfile_topipe(FILE *, int);
int filter_function(char*, char *, int, int);

int main(int argc, char*argv[]){
    // Controlli Preliminari:
    if(argc < 3 || argc > 5){
        display_error();
        exit(1);
    }
    
    program_info this;
    this.i_flag = -1; this.v_flag = -1;
    
    if(argc == 3){
        this.word = argv[1];
        this.file_pathname = argv[2];
    }
    else if(argc == 4){
        if(strcmp(argv[1],"-i") == 0) this.i_flag = 1;
        else if(strcmp(argv[1],"-v") == 0) this.v_flag = 1;
        else{ display_error(); exit(1); }
        
        this.word = argv[2];
        this.file_pathname = argv[3];
    }
    else if(argc == 5){
        if(strcmp(argv[1],"-i") == 0) this.i_flag = 1;
        else{ display_error(); exit(1); }
        if(strcmp(argv[2],"-v") == 0) this.v_flag = 1;
        else{ display_error(); exit(1); }
        
        this.word = argv[3];
        this.file_pathname = argv[4];
    }
    
    // Creazione PIPE:
    int first_pipe[2];
    if(pipe(first_pipe) == -1){ perror("pipe"); exit(1); }
    // Creazione FIFO:
    char *fifo_path = "/tmp/first_fifo";
    if(mkfifo(fifo_path, 0770) == -1) { perror("mkfifo"); exit(1); }
    
    // CODICE FIGLIO R:
    pid_t reader_pid = fork();
    if(reader_pid == -1){ perror("fork"); exit(1); }
    if(reader_pid == 0){
        // Apertura File da leggere:
        FILE *file_stream = fopen(this.file_pathname,"r");
        if(file_stream == NULL){ perror("fopen"); exit(1); }
        // Chiusura canale lettura PIPE:
        close(first_pipe[0]);
        // Lettura File e Scrittura su PIPE:
        fromfile_topipe(file_stream, first_pipe[1]);
        
        // Invio stringa terminatore:
        if(write(first_pipe[1], "END", KB) == -1){ perror("write"); exit(1); }
        
        exit(0);
    }
    
    // CODICE FIGLIO F:
    pid_t filter_pid = fork();
    if(filter_pid == -1){ perror("fork"); exit(1); }
    if(filter_pid == 0){
        // Apertura FIFO:
        int descrittore_fifo = open(fifo_path, O_WRONLY);
        if(descrittore_fifo == -1){ perror("open"); exit(1); }
        // Chiusura canale scrittura PIPE:
        close(first_pipe[1]);
        
        char buffer_filter[KB];
        while(1){
            // Lettura da PIPE:
            if(read(first_pipe[0], buffer_filter, KB) == -1){ perror("read"); exit(1); }
            // Controllo stringa terminatore:
            if(strcmp(buffer_filter,"END") == 0){
                // Invio stringa terminatore:
                if(write(descrittore_fifo, "END", KB) == -1){ perror("write"); exit(1); }
                // Chiusura FIFO:
                close(descrittore_fifo);
                
                exit(0);
            }
            // Filtro ed eventuale scrittura su FIFO:
            if(filter_function(this.word, buffer_filter, this.i_flag, this.v_flag) == 1){
                if(write(descrittore_fifo, buffer_filter, KB) == -1){ perror("write"); exit(1); }
            }
        }
    }
    
    // CODICE FIGLIO W:
    pid_t writer_pid = fork();
    if(writer_pid == -1){ perror("fork"); exit(1); }
    if(writer_pid == 0){
        // Apertura FIFO:
        int descrittore_fifo = open(fifo_path, O_RDONLY);
        if(descrittore_fifo == -1){ perror("open"); exit(1); }
        
        char buffer_writer[KB];
        while(1){
            // Lettura da FIFO:
            if(read(descrittore_fifo, buffer_writer, KB) == -1){ perror("read"); exit(1); }
            // Controllo stringa terminatore:
            if(strcmp(buffer_writer,"END") == 0){
                // Chiusura FIFO:
                close(descrittore_fifo);
                
                exit(0);
            }
            else printf("%s",buffer_writer);
        }
    }
    
    // CODICE PADRE P:
    waitpid(reader_pid,NULL,0);
    waitpid(filter_pid,NULL,0);
    waitpid(writer_pid,NULL,0);
    
    if(remove(fifo_path) == -1){ perror("remove"); exit(1); }
    
    exit(0);
}

// Stampa Errore:
void display_error(){
    printf("\nErrore, utilizzare la sintassi corretta: \n");
    printf("my-fgrep [-i] [-v] <word> <file_pathname>\n\n");
}

// Prende in input uno stream file e lo scrive su una pipe:
void fromfile_topipe(FILE *file_stream, int pipe_descriptor){
    char buffer[KB];
    // Lettura File:
    while(fgets(buffer, KB, file_stream) != NULL){
        // Scrittura su PIPE:
        if(write(pipe_descriptor, buffer, KB) == -1){ perror("write"); exit(1); }
    }
}

// Prende in input una parola, una stringa e delle opzioni e restituisce 1 o -1 in base alla computazione:
int filter_function(char* parola, char *stringa, int i_flag, int v_flag){
    if(i_flag == 1){
        if(v_flag == 1){
            if(strcasestr(stringa,parola) != NULL) return -1;
            else return 1;
        }
        else if(v_flag == -1){
            if(strcasestr(stringa,parola) != NULL) return 1;
            else return -1;
        }
        else return ERROR;
    }
    else if(i_flag == -1){
        if(v_flag == 1){
            if(strstr(stringa,parola) != NULL) return -1;
            else return 1;
        }
        else if(v_flag == -1){
            if(strstr(stringa,parola) != NULL) return 1;
            else return -1;
        }
        else return ERROR;
    }
    return ERROR;
}
