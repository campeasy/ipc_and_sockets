#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define IP       "127.0.0.1"  // Indirizzo IP su cui effettuare il bind
#define PORT     1234         // Porta su cui effettuare il bind
#define BACKLOG  1            // Backlog su listen()
#define DIM      256          // Dimensione buffer
#define CONN_MAX 1            // Richieste massime che la socket soddisfa

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

int main(){
    int descrittore_socket;
    int descrittore_connessa;
    int connections = 0;
    struct sockaddr_in indirizzo_server;
    struct sockaddr_in indirizzo_mittente;
    socklen_t size_mittente = sizeof(indirizzo_mittente);
    char buffer_recv[DIM];
    char buffer_send[DIM];
    
    // CREAZIONE SOCKET:
    descrittore_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    if(descrittore_socket == -1){
        closing_function("socket", descrittore_socket);
        exit(1);
    }
    printf("\n\n[SUCCESSO] Creazione/Apertura socket \n");
    // BINDING SOCKET:
    create_address(&indirizzo_server, PORT, IP);
    
    if(bind(descrittore_socket, (struct sockaddr *)&indirizzo_server, sizeof(indirizzo_server)) == -1){
        closing_function("bind", descrittore_socket);
        exit(1);
    }
    printf("[SUCCESSO] Binding descrittore/indirizzo socket \n");
    // ASCOLTO SU SOCKET:
    if(listen(descrittore_socket, BACKLOG) == -1){
        closing_function("listen", descrittore_socket);
        exit(1);
    }
    printf("[SUCCESSO] Ascolto su socket \n");
    // INSTAURAZIONE CONNESSIONE:
    while(1){
        strncpy(buffer_recv,"",DIM); // Reset buffer lettura
        strncpy(buffer_send,"",DIM); // Reset buffer scrittura
        
        descrittore_connessa = accept(descrittore_socket, (struct sockaddr *)&indirizzo_mittente, &size_mittente);
        if(descrittore_connessa != -1){
            connections++; // Incremento conteggio richieste servite
            printf("[SUCCESSO] Connessione con client instaurata\n----------\n");
    
            // Elaborazione su socket connessa:
            recv(descrittore_connessa, buffer_recv, DIM, 0);
            printf("%s", buffer_recv);
            strncpy(buffer_send,buffer_recv,DIM);
            send(descrittore_connessa, buffer_send, DIM, 0);
        }
        else perror("accept");
        if(connections == CONN_MAX) break;
    }
    // TERMINAZIONE PROGRAMMA:
    printf("\n----------\n[SUCCESSO] Terminazione programma \n\n");
    closing_function(NULL, descrittore_socket);
    exit(0);
}
