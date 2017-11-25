// ------------------------------------------------------------
// Salvatore Campisi
// C PROGRAMMING
// Anno Accademico 2017/2018
// Argomento: Unix Programming - File Mappati
// ------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

int main(){
    // Apertura File:
    char *pathname = "file/2.txt";
    int descrittore_file = open(pathname,O_RDWR);
    // Raccolta Informazioni File:
    struct stat info;
    fstat(descrittore_file, &info);
    // Mappatura in memoria:
    char *pointer = NULL;
    pointer = (char *) mmap(NULL, info.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, descrittore_file, 0);
    if(pointer == NULL){ perror("mmap"); exit(1); }
    // Lettura del File dalla memoria:
    for(int i=0; i<info.st_size; i++){
        printf("%c",pointer[i]);
    }
    exit(0);
}
