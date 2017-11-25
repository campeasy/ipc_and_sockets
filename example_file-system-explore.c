// ------------------------------------------------------------
// Salvatore Campisi
// C PROGRAMMING
// Anno Accademico 2017/2018
// Argomento: Unix Programming - Esplorazione File System
// ------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define KB 1024

void recursive_print(char *pathname, int level){
    DIR *directory_stream;
    struct dirent *actual;
    struct stat info;
    char nuovopathname[KB];
    
    directory_stream = opendir(pathname);
    if(directory_stream == NULL){ return; }
    
    // Stampa tutti i file:
    while((actual = readdir(directory_stream)) != NULL){
        // Determinazione nuovo Pathname:
        strncpy(nuovopathname,"",KB);
        strncat(nuovopathname,pathname,KB);
        strncat(nuovopathname,"/",KB);
        strncat(nuovopathname,actual->d_name,KB);
        
        // Raccoglie informazioni sulla voce attuale:
        stat(nuovopathname,&info);
        
        // Se è un file stampa:
        if(S_ISREG(info.st_mode)){
            for(int i=0; i<level; i++) printf("     ");
            printf(" | ");
            printf("%s \n",actual->d_name);
        }
    }
    
    // Riavvolgi lo Stream:
    rewinddir(directory_stream);
    
    // Stampa tutte le cartelle:
    while((actual = readdir(directory_stream)) != NULL){
        // Determinazione nuovo Pathname:
        strncpy(nuovopathname,"",KB);
        strncat(nuovopathname,pathname,KB);
        strncat(nuovopathname,"/",KB);
        strncat(nuovopathname,actual->d_name,KB);

        // Raccoglie informazioni sulla voce attuale:
        stat(nuovopathname, &info);
        
        // Se è una directory, stampa e richiama la procedura:
        if(S_ISDIR(info.st_mode)){
            if(strcmp(actual->d_name,".") != 0 && strcmp(actual->d_name,"..") != 0){
                for(int i=0; i<level; i++) printf("     ");
                printf(" | ");
                printf("%s : \n",actual->d_name);
                
                recursive_print(nuovopathname,level+1);
            }
        }
    }
    return;
}

int main(int argc, char *argv[]){
    if(argc < 2) exit(1);
    // Pathname della directory da analizzare:
    char *pathname = argv[1];
    
    recursive_print(pathname,0);
    
    exit(0);
}
