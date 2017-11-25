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

#define PORT 5003   // Numero di porta
#define DIM 256     // Dimensione massima buffer
#define K 1         // Numero connesioni massime

// SERVER:
int main(){
    // CREAZIONE-APERTURA SOCKET:
    int descrittore_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(descrittore_socket == -1){
        perror("socket");
        exit(1);
    }
    printf("\nAPERTURA SOCKET... [CORRETTO]\n");
    
    // CREAZIONE INDIRIZZO SOCKET:
    struct sockaddr_in indirizzo_server;
    indirizzo_server.sin_family = AF_INET;
    indirizzo_server.sin_port = htons(PORT);
    indirizzo_server.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("CREAZIONE INDIRIZZO SOCKET... [CORRETTO]\n");
    
    // BINDING DESCRITTORE-INDIRIZZO:
    if(bind(descrittore_socket, (struct sockaddr *) &indirizzo_server, sizeof(indirizzo_server)) == -1){
        perror("bind");
        close(descrittore_socket);
        exit(1);
    }
    printf("BINDING DESCRITTORE-SOCKET... [CORRETTO]\n");
    
    // COMUNICAZIONE AL SO DISPONIBILITÀ ALLA CONNESSIONE:
    if(listen(descrittore_socket, K) == -1){
        perror("listen");
        close(descrittore_socket);
        exit(1);
    }
    printf("ASCOLTO SULLA SOCKET... [CORRETTO]\n");
    // Variabili di supporto:
    struct sockaddr_in temp_mittente;
    socklen_t temp_mittente_size = sizeof(temp_mittente);
    char buffer[DIM];
    int flag = 0, connection_max = 0;
    
    printf("Socket in attesa di connessioni...\n\n");
    while(1){
        // Instaurazione della connesione:
        int descrittore_socket_connessa = accept(descrittore_socket, (struct sockaddr *) &temp_mittente, &temp_mittente_size);
        if(descrittore_socket_connessa != -1){
            printf("Connesione instaurata correttamente - Ricezione Dati:\n\n");
            // Connesione instaurata:
            flag = 0; connection_max++;
            // Ricezioni dati dalla socket:
            while(flag != 1){
                recv(descrittore_socket_connessa, buffer, DIM, 0);
                printf("%s",buffer);
                if(strcmp(buffer,"END") == 0 || strcmp(buffer,"END\n") == 0) flag = 1;
            }
            // Terminazione elaborazione:
            close(descrittore_socket_connessa);
            // Controllo connessioni max:
            if(connection_max == K){
                close(descrittore_socket);
                
                printf("CHIUSURA SOCKET... [CORRETTO]\n");
                
                exit(0);
            }
        }
        else perror("accept");
    }
}
