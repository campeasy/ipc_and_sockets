#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DIM 256  // Dimensione buffer

void closing_function(char *error, int descrittore_socket){
    if(error != NULL) perror("error");
    if(descrittore_socket != -1) close(descrittore_socket);
    return;
}

void create_address(struct sockaddr_in *temp, int port, char *ipv4_addr){
    temp->sin_family = AF_INET;
    temp->sin_port = htons(port);
    temp->sin_addr.s_addr = inet_addr(ipv4_addr);
    return;
}

int main(int argc, char *argv[]){
    // CONTROLLI SULL'INVOCAZIONE DEL PROGRAMMA:
    if(argc < 3){ printf("\nSintassi corretta: ./client.out indirizzo_ip porta \n"); exit(1); }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    
    int descrittore_socket;
    struct sockaddr_in indirizzo_destinatario;
    create_address(&indirizzo_destinatario, port, ip);
    int size_destinatario = sizeof(indirizzo_destinatario);
    char buffer_send[DIM];
    char buffer_recv[DIM];
    
    // CREAZIONE SOCKET:
    descrittore_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(descrittore_socket == -1){
        closing_function("socket", descrittore_socket);
        exit(1);
    }
    printf("\n\n[SUCCESSO] Creazione/Apertura socket \n");
    // INSTAURAZIONE DELLA CONNESSIONE:
    if(connect(descrittore_socket, (struct sockaddr *)&indirizzo_destinatario, size_destinatario) == -1){
        closing_function("socket", descrittore_socket);
        exit(1);
    }
    printf("[SUCCESSO] Connessione con server instaurata \n");
    
    while(1){
        strncpy(buffer_send, "", DIM); // Reset buffer scrittura
        strncpy(buffer_recv, "", DIM); // Reset buffer lettura
        
        printf("----------\nInserire i dati da inviare al server: \n");
        fgets(buffer_send, DIM, stdin);
        
        send(descrittore_socket, buffer_send, DIM, 0);
        recv(descrittore_socket, buffer_recv, DIM, 0);
        
        if(strcmp(buffer_send, buffer_recv) == 0){ printf("\n----------\n[SUCCESSO] Richiesta Soddisfatta \n"); break; }
    }
    
    // TERMINAZIONE PROGRAMMA:
    printf("[SUCCESSO] Terminazione programma \n\n");
    closing_function(NULL, descrittore_socket);
    exit(0);
}
