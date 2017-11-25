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

// SERVER:
int main(){
    // CREAZIONE-APERTURA SOCKET:
    int descrittore_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(descrittore_socket == -1){
        perror("socket");
        exit(1);
    }
    printf("\n\nAPERTURA SOCKET... [CORRETTO]\n");
    
    // COSTRUZIONE INDIRIZZO SERVER:
    struct sockaddr_in indirizzo_server;
    indirizzo_server.sin_family = AF_INET;
    indirizzo_server.sin_port = 6550;
    indirizzo_server.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("CREAZIONE INDIRIZZO SERVER... [CORRETTO]\n");
    
    // BINDING SOCKET-INDIRIZZO:
    if(bind(descrittore_socket, (struct sockaddr *)&indirizzo_server, sizeof(indirizzo_server)) != 0){
        perror("bind");
        close(descrittore_socket);
        exit(1);
    }
    printf("BINDING DESCRITTORE-INDIRIZZO... [CORRETTO]\n");
    
    // VARIABILE DI SUPPORTO:
    // Indirizzo da riempire ad ogni pacchetto ricevuto:
    struct sockaddr_in temp_mittente;
    socklen_t temp_mittente_size = sizeof(temp_mittente);
    // Buffer da riempire con il Payload:
    char buffer[DIM];
    
    // RICEZIONE DATI SULLA SOCKET:
    printf("Socket in attesa di dati: \n");
    while(1){
        recvfrom(descrittore_socket, buffer, DIM, 0, (struct sockaddr *)&temp_mittente, &temp_mittente_size);
        printf("%s", buffer);
        
        // CONTROLLO STRINGA TERMINATORE:
        if(strcmp("END", buffer) == 0 || strcmp("END\n", buffer) == 0){
            close(descrittore_socket);
            
            printf("TERMINAZIONE COMUNICAZIONE... [CORRETTO] \n");
            exit(1);
        }
    }
}
