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

/*Recuperer le nom du chemin a partir du message envoye par le client qui se presente sous la forme :
  GET /chemin HTTP/1.1, on ne veut pas du premier '/' car il indique le repertoire racine du serveur */
char* getFilepath(char* client_msg){
  int prem_slash = 0; /*Si prem_slash == 1 alors le premier / a ete lu*/
  int i = 0, j = 0;
  char char_lu;
  char* filepath;

  filepath = malloc(sizeof(char) * 50);
  memset(filepath, 0, sizeof(char) * 50);

  while( (char_lu = client_msg[i]) != '\0' ){
    printf("Charactere lu %c\n", char_lu);
    if(char_lu == '/' && prem_slash == 0){
      prem_slash++;
      i++;
      continue;
    }

    if(prem_slash == 1){
      if(char_lu != ' '){
        filepath[j] = char_lu;
        j++;
        printf("ici, valeur de i : %d\n", i);
      }
      else
        break;
    }
    i++;
  }
  filepath[i] = '\0';
  return filepath;
}

/*Lui donner un filepath copie, car strtok va detruire le filepath completement,
  le filepath ne contiendra plus que la premiere chaine de caractere, celle qui
  se trouve avant le premier slash.
  Ex : samples/toto/file.pdf => filepath ne contiendra plus que samples a la fin
  */
char* getExtension(char* filepath){
  const char slash[2] = "/";
  const char point[2] = ".";
	char *token_filename, *token_extension;
  char filename[50], *extension;

  extension = malloc(sizeof(char) * 50);

  /*Recuperer nom du fichier*/
  token_filename = strtok(filepath, slash);
	while(token_filename != NULL){
		strcpy(filename, token_filename);
		token_filename = strtok(NULL, slash);
	}

  /*Recuperer extension*/
  token_extension = strtok(filename, point);
  while(token_extension != NULL){
		strcpy(extension, token_extension);
		token_extension = strtok(NULL, point);
	}

  return extension;
}

int main(int argc, char * argv[]){
  struct sockaddr_in sin;
  int sock_connexion, sock_communication, fd, retour_read;
  char msg[2000], http_header[50], download_buffer[BUFFERSIZE], filepath_cpy[50];
  char *filepath, *extension, *type;
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

  filepath = getFilepath(msg);
  strcpy(filepath_cpy, filepath);
  extension = getExtension(filepath_cpy);
  type = getType(extension);

  /*application/pdf*/
  /*TODO : Corriger cas text/plain*/
  memset(http_header, 0, sizeof(http_header));

  strcpy(http_header, "HTTP/1.1 200 OK\nContent-Type: ");
  strcat(http_header, type);
  strcat(http_header, "\n\n"); /*Important de laisser une ligne vide entre le Content-Type et le contenu du fichier*/

  if(write(sock_communication, http_header, sizeof(http_header)) < 0){
    perror("Erreur d'ecriture sur la socket\n");
    return errno;
  }

  if((fd = open(filepath, O_RDWR, 0644)) == -1){
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
  free(filepath);
  free(extension);
  printf("Fin de communication.\nTerminaison du serveur.\n");
  return 0;
}
