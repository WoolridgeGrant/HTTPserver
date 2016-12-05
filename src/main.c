#define _XOPEN_SOURCE 700
#define PORTSERV 7100

#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>


int main(int argc, char * argv[]){
  struct sockaddr_in sin;
  int sock_connexion, sock_communication, taille_addr = sizeof(sin);
  char msg[2000], msg1[50];

  if( (sock_connexion = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("Erreur de creation de socket\n");
    return errno;
  }

  /* Initialisation de la socket */
  memset((char *) &sin, 0, sizeof(sin));
  sin.sin_addr.s_addr =  htonl(INADDR_ANY);/* inet_addr("127.0.0.1");*/
  sin.sin_port = htons(PORTSERV);
  sin.sin_family = AF_INET;

  printf("Avant bind\n");

  if( bind(sock_connexion, (struct sockaddr *) &sin, sizeof(sin)) == -1){
    perror("Erreur de nommage de la socket\n");
    return errno;
  }
  /* Fin initialisation de socket */

  /*On ecoute sur la socket*/
	listen(sock_connexion, 5);

  if( (sock_communication = accept(sock_connexion, (struct sockaddr*) &sin, &taille_addr)) == -1 ){
    perror("Erreur accept\n");
    return errno;
  }

  if(read(sock_communication, msg, sizeof(msg)) < 0){
    perror("Erreur de lecture de la socket\n");
    return errno;
  }
	printf("msg recu : %s", msg);

	strcpy(msg1, "HTTP/1.1 200 OK\nContent-Type: text/html\n\nCoucou");
	if(write(sock_communication, msg1, sizeof(msg1)) < 0){
	perror("Erreur d'ecriture sur la socket\n");
    return errno;
	}

  shutdown(sock_communication, SHUT_WR | SHUT_RD);
  close(sock_communication);
  close(sock_connexion);
  printf("Fin de communication.\nTerminaison du serveur.\n");
  return 0;
}
