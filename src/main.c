#define _XOPEN_SOURCE 700

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

#include "../include/struct.h"

/*Le serveur prend en argument le numero de port sur lequel il ecoute,
  le nombre de clients qu'il peut accepter simultanement
  et un argument determine en question 5*/

/*  TODO :
    1. Recuperer le port sur lequel le serv ecoute ainsi que le nb de clients max
    2. Lancer un thread a chaque connexion de client
    3. Faire tous les trucs pour accepter une connexion sur la routine du thread
    4. Recuperer les deux lignes envoyees par le navigateur en parcourant caractere par caractere pour trouver \n
    5. Parcourir le tableau des Content-Type et recuperer la bonne cle
    6. Envoyer les deux lignes
    7. Envoyer le contenu du fichier
    */

mime mimes[800];
int size_mimes = 0;

char* getType(char *ext){
	int indice = 0;

	while(indice <= size_mimes){
		if(strcmp(ext, mimes[indice].ext) == 0){
      return mimes[indice].type;
		}
    else
			indice++;
	}

  return NULL;
}

int addTypes(){
  int i = 0, j = 0, retour_read = 0;
	int fd;
	char char_lu;
	int presence_extension = 0;

	char type[100];
	char extension[100];

	memset(mimes, 0, sizeof(mimes));
	memset(type, 0, sizeof(type));
	memset(extension, 0, sizeof(extension));

  if((fd = open("mimes-types", O_RDWR, 0644)) == -1){
      perror("Erreur ouverture du fichier");
      return errno;
  }

  while( (retour_read = read(fd, &char_lu, sizeof(char))) > 0){

    if(retour_read == -1){
      perror("Erreur de lecture\n");
      return errno;
    }
    /*Ligne de commentaire dans le mime.types*/
    if(char_lu == '#'){
      while(char_lu != '\n'){
        read(fd, &char_lu, sizeof(char));
      }
      continue;
    }

    if(char_lu != '\n'){
			if(char_lu == '\t'){
				presence_extension = 1;
			}
      else if(presence_extension == 0){
				type[i] = char_lu;
				i++;
			}

			if(presence_extension == 1 && char_lu != '\t'){
				if(char_lu == ' '){
					j = 0;
					strcpy(mimes[size_mimes].ext, extension);
					strcpy(mimes[size_mimes].type, type);
					memset(extension, 0, sizeof(extension));
					size_mimes++;
				}
        else {
					extension[j] = char_lu;
					j++;
				}
			}
		}
    else{
			i = 0;
			j = 0;
			if(presence_extension == 1){
				strcpy(mimes[size_mimes].ext, extension);
				strcpy(mimes[size_mimes].type, type);
				size_mimes++;
			}
			memset(type, 0, sizeof(type));
			memset(extension, 0, sizeof(extension));
			presence_extension = 0;
		}
  }

  return 1;
}

int main(int argc, char * argv[]){
  struct sockaddr_in sin;
  int sock_connexion, sock_communication, fd, retour_read;
  char msg[2000], http_header[50], download_buffer[BUFFERSIZE];
	unsigned int taille_addr = sizeof(sin);

  addTypes();

  if( (sock_connexion = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("Erreur de creation de socket\n");
    return errno;
  }

  /* Initialisation de la socket */
  memset((char *) &sin, 0, sizeof(sin));
  sin.sin_addr.s_addr =  htonl(INADDR_ANY);/* inet_addr("127.0.0.1");*/
  sin.sin_port = htons(PORTSERV);
  sin.sin_family = AF_INET;

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
	printf("%s", msg);

  /*application/pdf*/
  memset(http_header, 0, sizeof(http_header));
  strcpy(http_header, "HTTP/1.1 200 OK\nContent-Type: application/pdf\n\n"); /*Important de laisser une ligne vide entre le Content-Type et le contenu du fichier*/
  if(write(sock_communication, http_header, sizeof(http_header)) < 0){
    perror("Erreur d'ecriture sur la socket\n");
    return errno;
  }


  if((fd = open("./samples/test.pdf", O_RDWR, 0644)) == -1){
    perror("Erreur ouverture du fichier");
    return errno;
  }

  memset(download_buffer, 0, sizeof(download_buffer));
  printf("download_buffer : %s\n", download_buffer);
  while ( (retour_read = read(fd, download_buffer, sizeof(download_buffer))) > 0){
    if(retour_read == -1){
      perror("Erreur de lecture du fichier\n");
      return errno;
    }

    printf("download_buffer dans while : %s\n", download_buffer);
    /*On ecrit la taille qu'on lit sinon on aura des caracteres indesirables*/
    if(write(sock_communication, download_buffer, retour_read) < 0){
      perror("Erreur d'ecriture sur la socket\n");
      return errno;
    }
  }

  close(fd);

  shutdown(sock_communication, SHUT_WR | SHUT_RD);
  close(sock_communication);
  close(sock_connexion);
  printf("Fin de communication.\nTerminaison du serveur.\n");
  return 0;
}
