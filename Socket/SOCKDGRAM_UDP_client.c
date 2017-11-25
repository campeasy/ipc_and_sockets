// Salvatore Campisi, 16/11/17
// Comunicazione tra due SOCK_DGRAM

// HEADERS:
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DIM 256

// CLIENT:
int main(){
    // CREAZIONE-APERTURA SOCKET:
    int descrittore_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(descrittore_socket == -1){
        perror("socket");
        exit(1);
    }
    printf("\n\nAPERTURA SOCKET... [CORRETTO]\n");
    
    // COSTRUZIONE INDIRIZZO A CUI INVIARE I DATI:
    struct sockaddr_in indirizzo_destinatario;
    indirizzo_destinatario.sin_family = AF_INET;
    indirizzo_destinatario.sin_port = 6550;
    indirizzo_destinatario.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("COSTRUZIONE INDIRIZZO DESTINATARIO... [CORRETTO]\n");
    
    // RESET BUFFER:
    char buffer[DIM];
    strncpy(buffer, "", DIM);
    
    // INVIO DATI:
    printf("INVIO DATI...\n");
    while(1){
        fgets(buffer, DIM, stdin);
        if(sendto(descrittore_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&indirizzo_destinatario, sizeof(indirizzo_destinatario)) == -1){
            perror("sendto");
            close(descrittore_socket);
            exit(1);
        }
        if(strcmp(buffer, "END\n") == 0){
            printf("Dati inviati correttamente. \n");
            close(descrittore_socket);
            exit(0);
        }
    }
}
