
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
#include <pthread.h>


#include "../include/initialisation.h"
#include "../include/t_routine.h"
#include "../include/globals.h"
#include "../include/log.h"

/*Va read les requetes d'une meme connexion puis creer des threads routines
  answer pour y repondre, en leur donnant en argument le thread qu'ils doivent
  join a chaque fois. Pour le premier l'id ce thread en argument sera egal a 0
  de sorte a ce qu'il sache qu'il n'y a personne a attendre
  On lui donne en argument la requete completee*/
void *routine_read_req(void* arg){
  requete *req = (requete*)arg;
  requete *req_tmp;
  pthread_t *t_to_join = malloc(sizeof(pthread_t));
  pthread_t tab_tid[100];
  int size_tab = 0;
  int i = 0;
  *t_to_join = 0;

  req_tmp = malloc(sizeof(struct requete));
  req_tmp->thread_to_join = *t_to_join;
  req_tmp->soc_com = req->soc_com;
  req_tmp->sin = req->sin;
  if(read(req_tmp->soc_com, req_tmp->msg, sizeof(req_tmp->msg)) < 0){
    perror("Erreur de lecture de la socket\n");
    /*return errno;*/
  }

  /*ici*/

  if (pthread_create(t_to_join, NULL, routine_answer, (void*)req_tmp) != 0) {
    printf("pthread_create\n");
    exit(1);
  }

  tab_tid[i] = *t_to_join;
  size_tab++;

  for(i = 0; i < size_tab; i++){
    pthread_join(tab_tid[i], NULL);
  }

  shutdown(req->soc_com, SHUT_WR | SHUT_RD);
  close(req->soc_com);

  free(t_to_join);
  pthread_exit(NULL);
}

/*ERRNO IS THREAD-SAFE
  errno is thread-local; setting it in one thread does not affect its value in any other thread.*/
void *routine_answer(void* arg) {
  requete *req = (requete*)arg;
  struct sockaddr_in sin;
  unsigned int taille_addr = sizeof(sin);
  int fd, retour_read;
  int erreur = 0; /*s'il y a erreur alors egal a 1*/
  char download_buffer[BUFFERSIZE];
  char http_header[50];
  char filepath_cpy[50];
  char http_code[5] = "200 ";
  char http_msg_retour[50] = "OK\n";
  char *filepath, *extension, *type;

  if(req->thread_to_join != 0){
    pthread_join(req->thread_to_join, NULL);
  }

  filepath = getFilepath(req->msg);
  strcpy(filepath_cpy, filepath);
  extension = getExtension(filepath_cpy);
  type = getType(extension);

  if((fd = open(filepath, O_RDWR, 0644)) == -1){
    erreur = 1;
    perror("Erreur ouverture du fichier");
    if(errno == EACCES){ /*Permissions insuffisantes pour acceder au fichier*/
      strcpy(http_code, "403 ");
      strcpy(http_msg_retour, "Forbidden\n");
    }
    else if(errno == ENOENT){ /*Fichier inexistant*/
      strcpy(http_code, "404 ");
      strcpy(http_msg_retour, "Not Found\n");
    }
    /*return errno;*/
  }

  strcpy(req->http_code, http_code);

  /*TODO : Corriger cas text/plain*/
  memset(http_header, 0, sizeof(http_header));

  strcpy(http_header, "HTTP/1.1 ");
  strcat(http_header, http_code);
  strcat(http_header, http_msg_retour);
  if(!erreur){
    strcat(http_header, "Content-Type: ");
    strcat(http_header, type);
    strcat(http_header, "\n\n"); /*Important de laisser une ligne vide entre le Content-Type et le contenu du fichier*/
  }

  printf("Header renvoye au client : \n%s\n", http_header);
  if(write(req->soc_com, http_header, sizeof(http_header)) < 0){
    perror("Erreur d'ecriture sur la socket\n");
    /*return errno;*/
  }

  req->size_file = 0;


  if(!erreur){
    memset(download_buffer, 0, sizeof(download_buffer));
    while ( (retour_read = read(fd, download_buffer, sizeof(download_buffer))) > 0){
      if(retour_read == -1){
        perror("Erreur de lecture du fichier\n");
        /*return errno;*/
      }

      /*On ecrit la taille qu'on lit sinon on aura des caracteres indesirables*/
      if(write(req->soc_com, download_buffer, retour_read) < 0){
        perror("Erreur d'ecriture sur la socket\n");
        /*return errno;*/
      }
    }

    /*taille fichier demandé*/
    req->size_file = lseek(fd, 0, SEEK_END);

    close(fd);
  }

  addLog(req);

  free(filepath);
  free(extension);
  /*free(req);*/

  pthread_exit(NULL);
}
