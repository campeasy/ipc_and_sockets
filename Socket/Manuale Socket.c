// ------------------------------------------------------------
// Salvatore Campisi
// C UNIX PROGRAMMING - MANUALE
// Anno Accademico 2017/2018
// Argomento: Socket
// ------------------------------------------------------------

// SOCKET: "canale" logico di comunicazione tra processi, sia remoti, sia residenti sullo stesso sistema.
// Dopo aver creato e aperto una Socket, essa è accessibile come un normale file UNIX.
// ------------------------------
// ELEMENTI CARATTERIZZANTI DI UNA SOCKET:
// ------------------------------
// DOMINIO della Socket: le socket dello STESSO dominio possono comunicare tra loro.
// 1. PF_UNIX (Socket Locali)
// 2. PF_INET (Socket Internet)
// ------------------------------
// TIPO di Socket: tipologia di comunicazione che la socket implementa.
// 1. SOCK_STREAM: connnection_oriented (protocollo a livello di trasporto predefinito: TCP).
// 2. SOCK_DGRAM: connection_less (protocollo a livello di trasporto predefinito: UDP).
// ------------------------------
// PROTOCOLLI della socket: protocolli che la socket utilizzerà (rete,trasporto).

// CHIAMATE DI SISTEMA:
// ------------------------------
// (1) Creazione/Apertura di una socket:
// HEADERS:
   #include <sys/types.h> #include <sys/socket.h>
// PROTOTIPO:
   int socket(int domain, int type, int protocol);
// PARAMETRI:
   'domain'   // (dominio della socket) PF_UNIX, PF_INET
   'type'     // (tipo della socket) SOCK_STREAM, SOCK_DGRAM
   'protocol' // (protocolli utilizzati dalla socket) se 0 = protocollo di default del dominio/tipo.
// VALORE RESTITUITO: file descriptor della socket creata in caso di successo, -1 in caso di errore.
// ------------------------------
// (2) Binding (collegamento) del file descriptor di una socket ad uno specifico indirizzo:
// La chiamata assegna un nome (indirizzo) ad una socket senza nome (indirizzo).

// HEADERS:
   #include <sys/types.h>  #include <sys/socket.h>
// PROTOTIPO:
   int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
// PARAMETRI:
   'sockfd'  // descrittore file della socket.
   'my_addr' // puntatore ad una struttura sockaddr (contenente l'INDIRIZZO della socket).
   'addrlen' // dimensione in byte della struttura my_addr.
// VALORE RESTITUITO: 0 in caso di successo, -1 in caso di errore.

// NOTE:
// struct sockaddr: la struct sockaddr si SPECIALIZZA in base al DOMINIO della socket.
// PF_UNIX: struct sockaddr_un  (TIPO)
//          sun_family = AF_UNIX,
//          sunpath[] = (char[]) Pathname della socket.
// PF_INET: struct sockaddr_in  (TIPO)
//          sin_family = AF_INET,
//          sin_port = (int 16 bit) porta // NB: utilizzare htons() per convertire big/little endian.
//          sin_addr.s_addr = (int 32 bit) indirizzo IPV4.
// In QUALSIASI chiamata di sistema figurerà il GENERICO tipo sockaddr, occorre "castare" il tipo
// utilizzando: (struct sockaddr *) &genericAddr
// genericAddr può essere di tipo: sockaddr_un , sockaddr_in
// ------------------------------
// (3) Connessione di una socket ad un'altra socket (remota o locale): (CLIENT)
// HEADERS:
   #include <sys/types.h>  #include <sys/socket.h>
// PROTOTIPO:
   int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
// PARAMETRI:
   'sockfd'    // descrittore file della socket.
   'serv_addr' // puntatore ad una struttura sockaddr (contenente l'indirizzo della socket a cui associarsi/connettersi).
   'addrlen'   // dimensione in byte della struttura serv_addr.
// VALORE RESTITUITO: 0 in caso di successo, -1 in caso di errore.

// NOTE:
// Il comportamento della chiamata si DIFFERENZIA in base al TIPO della socket sockfd:
// SOCK_DGRAM: (NON NECESSARIA) serv_addr è la socket a cui ASSOCIARSI (a cui inviare e da cui ricevere dati),
//             la chiamata può essere effettuata più volte, poichè il suo successo non GARANTISCE
//             l'effettiva connesione con la socket remota, ma una semplice associazione locale.
//             (Abbiamo SOLAMENTE specificato a CHI inviare/da CHI ricevere i dati su quella socket).
// SOCK_STREAM: serv_addr è la socket a cui CONNNETTERSI (a cui inviare e da cui ricevere dati su quella socket),
//              la chiamata cerca di effettuare una connessione con la socket remota, se la chiamata ha
//              successo la connessione è stata stabilita. Altrimenti fallisce.
// ------------------------------
// (4) Attesa della connessione: (SERVER) (per socket di tipo SOCK_STREAM)
// La chiamata comunica al SO che la socket è DISPOSTA ad accettare connessioni.
// Attenzione, NON che le stia effettivamente accettando, ma solo che è DISPOSTA a farlo.

// HEADER:
   #include <sys/socket.h>
// PROTOTIPO:
   int listen(int sockfd, int backlog);
// PARAMETRI:
   'sockfd'  // descrittore file della socket.
   'backlog' // specifica al SO il NUMERO di richieste di connessione che la socket sockfd può accodare fino a quando
             // sulla stessa socket sockfd non viene eseguita la chiamata di accettazione (accept).
// VALORE RESTITUITO: 0 in caso di successo, -1 in caso di errore.

// NOTE: se perviene una richiesta di connessione e la coda è piena, la connect del client fallirà con apposito errore.
// ------------------------------
// (5) Accettazione e Instaurazione della connessione: (SERVER) (per socket di tipo SOCK_STREAM)
// Dopo aver comunicato al SO che la socket sockfd è DISPOSTA ad accettare connessioni (listen),
// si attende una connessione effettiva da qualche processo remoto tramite le chiamata accept.
// Quando vi è una richiesta di connnesione in coda, la chiamata estrae la prima richiesta e crea un'altra socket con
// le STESSE proprietà della socket sockfd, CONNESSA alla socket (remota o non) che ha richiesto la connessione.
// Il descrittore file della NUOVA socket viene restituito come valore di ritorno.

// HEADERS:
   #include <sys/types.h>  #include <sys/socket.h>
// PROTOTIPO:
   int accept(int sockfd, struct sockaddr *new_addr, socklen_t *new_addrlen);
// PARAMETRI:
   'sockfd'      // descrittore file della socket su cui accettare connesioni.
   'new_addr'    // puntatore ad una struttura sockaddr che verrà riempita con l'indirizzo della NUOVA socket.
   'new_addrlen' // dimensione in byte della struttura new_addr.
// VALORE RESTITUITO: descrittore file della NUOVA socket in caso di successo, -1 in caso di errore.

// NOTE - Gestione delle nuove connessioni:
while(1){                                               while(1){
    newsockfd = accept(sockfd, ...);                        newsockfd = accept(sockfd, ...);
    if(newsockfd == -1){ stampa_errore(); }                 if(newsockfd == -1){ stampa_errore(); }
    if(fork() == 0){                                        elaborazione(newsockfd);
        close(sockfd);                                      close(newsockfd);
        elaborazione(newsockfd);                        }
        exit(0);
    }
    close(newsockfd);
}
// Nel caso in cui gestiamo le accept creando un processo figlio, nel processo figlio è possibile chiudere il
// descrittore file della vecchia socket sockfd, ed eseguire in "parallelo" l'elaborazione sulla nuova socket,
// mentre nel processo padre è possibile continuare ad "accettare" connessioni.
// Nel caso in cui gestiamo le accept in modo iterativo, dopo aver creato la nuova socket eseguiamo l'elaborazione
// e dopo aver terminato l'elaborazione per la nuova socket, la chiudiamo, e torniamo ad "accettare" connessioni.
// ------------------------------
// (7) Inviare/Scrivere dati SU una socket:
// Differenziamo il MODO di inviare/scrivere su una socket in base al TIPO della stessa.

// HEADERS:
   #include <sys/types.h>  #include <sys/socket.h>
// PROTOTIPI:
   int sendto(int sockfd, char *buffer, int nbytes, int flags, struct sockaddr *to_addr, int addrlen); // SOCK_DGRAM
   int send(int sockfd, char *buffer, int nbytes, int flags);                                          // SOCK_STREAM
// PARAMETRI:
   'sockfd'  // descrittore file della socket su cui scrivere i dati.
   'buffer'  // puntatore ad una area di memoria (array) che contiene i dati da scrivere sulla socket.
   'nbytes'  // specifica il numero di byte (caratteri) che si desidera scrivere sulla socket.
   'flags'   // in questo ambiente assumeremo che flags valga sempre 0.
   'to_addr' // puntatore ad una struttura sockaddr contenente l'indirizzo della socket a cui INVIARE i dati.
   'addrlen' // dimensione in byte della struttura to_addr.
// VALORE RESTITUITO: numero di byte inviati in caso di successo, -1 in caso di errore.

// NOTE: La chiamata send() PRESUPPONE che la socket sockfd sia GIÀ CONNESSA con una socket. (SOCK_STREAM).
//       La chiamata send(), con flags = 0, si comporta esattamente come la chiamata di sistema "write".
// ------------------------------
// (8) Ricevere/Leggere dati DA una socket:
// Differenziamo il MODO di ricevere/leggere da una socket in base al TIPO della stessa.

// HEADERS:
   #include <sys/types.h>  #include <sys/socket.h>
// PROTOTIPI:
   int recvfrom(int sockfd, char *buffer, int nbytes, int flags, struct sockaddr *from_addr, socklen_t *addrlen); // SOCK_DGRAM
   int recv(int sockfd, char *buffer, int nbytes, int flags);                                                     // SOCK_STREAM
// PARAMETRI:
   'sockfd'    // descrittore file della socket da cui leggere i dati.
   'buffer'    // puntatore ad una area di memoria temporanea (array) su cui copiare i dati letti dalla socket.
   'nbytes'    // specifica il numero di byte (caratteri) che si desidera leggere dalla socket.
   'flags'     // in questo ambiente assumeremo che flags valga sempre 0.
   'from_addr' // puntatore ad una struttura sockaddr contenente l'indirizzo della socket da cui RICEVERE i dati.
   'addrlen'   // dimensione in byte della struttura from_addr.
// VALORE RESTITUITO: numero di byte ricevuti in caso di successo, -1 in caso di errore.

// NOTE: La chiamata recvfrom tratta la dimensione dell'indirizzo come PUNTATORE di tipo socklen_t.
//       La chiamata recv() PRESUPPONE che la socket sockfd sia GIÀ CONNESSA con una socket. (SOCK_STREAM).
//       La chiamata recv(), con flags = 0, si comporta esattamente come la chiamata di sistema "read".
// ------------------------------
// (9) Chiusura di una socket:
// HEADER:
   #include <unistd.h>
// PROTOTIPO:
   int close(int sockfd);
// PARAMETRI:
   'sockfd' // descrittore file della socket da chiudere.
// VALORE RESTITUITO: 0 in caso di successo, -1 in caso di errore.

// NOTE: Se la socket sockfd è di tipo SOCK_STREAM, il kernel garantirà la trasmissione di qualasi dato ancora accodato.
// ------------------------------
// (10) Convertire un indirizzo IPv4 espresso come stringa nel rispettivo valore numerico:
// HEADER:
   #include <arpa/inet.h>
// PROTOTIPO:
   unsigned long inet_addr(const char *addr_ipv4);
// PARAMETRI:
   'addr_ipv4' // indirizzo ipv4 in notazione puntata "a.b.c.d" da convertire.
// VALORE RESTITUITO: corrispettivo unsigned long di addr_ipv4 in caso di successo, -1 in caso di errore.

// NOTE: il valore restituito è possibile impostarlo come valore di sin_addr.s_addr di una struttura sockaddr_in
// ------------------------------
// (11) Utilizzare il DNS: convertire un hostname in un indirizzo ipv4:
// Utilizzando il servizio DNS implementato in UNIX (BIND - Berkley Internet Name Domain),
// partendo da un hostname, se esso esiste, si ottiene il suo indirizzo ipv4.

// HEADERS:
   #include <arpa/inet.h>  #include <netdb.h>  #include <string.h>
// FUNZIONE:
   int dns_nametoipv4(char *hostname, char *ip_buffer, int dim_ip_buffer){
       struct hostent *supp = gethostbyname(hostname);
       if(supp == NULL) return -1;
       struct in_addr *ind = (struct in_addr *) supp->h_addr;
       if(ind == NULL) return -1;
       strncpy(ip_buffer, inet_ntoa(* ind), dim_ip_buffer);
       return 0;
   }
// PARAMETRI:
   'hostname'      // stringa contenente il nome dell'host di cui vogliamo ottenere l'indirizzo ipv4.
   'ip_buffer'     // puntatore ad una area di memoria temporanea (array) su cui copiare i dati letti dalla socket.
   'dim_ip_buffer' // dimensione massima che ip_buffer può assumere.
// VALORE RESTITUITO: 0 se l'indirizzo ipv4 è stato correttamente copiato su ip_buffer, -1 in caso di errore.
// ------------------------------
// (12) Socket Bloccante/Non Bloccante:
// Per default, le principali operazioni di una socket sono BLOCCANTI rispetto all'I/O.
// La chiamata "trasforma" le operazioni di una socket in operazioni NON BLOCCANTI.

// HEADERS:
   #include <sys/types.h>  #include <sys/socket.h>  #include <unistd.h>
   #include <fcntl.h>  #include <stdio.h>  #include <stdlib.h>
// PROTOTIPO:
   int fcntl(int sockfd, int cmd, int flag);
// PARAMETRI:
   'sockfd' // descrittore file della socket.
   'cmd'    // F_SETFL = setta il flag della socket a flag.
   'flag'   // O_NONBLOCK = flag che trasforma la socket in NON BLOCCANTE.
// VALORE RESTITUITO: != -1 in caso di successo, -1 in caso di errore.
// ------------------------------------------------------------
