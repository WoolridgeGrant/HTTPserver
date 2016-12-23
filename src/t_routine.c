#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
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
    requete *req_tmp = malloc(sizeof(struct requete));
    pthread_t *t_to_join = malloc(sizeof(pthread_t));
    pthread_t tab_tid[100];
    int size_tab = 0;
    int i = 0;

    int nb_req = 0;
    unsigned int cpt = 0;/*compteur sur les caractère de la rafale de requete*/
    int j = 0;           /*indice pour la separation de requete*/
    int nb_CRLF= 0;     /*nombre de '\r' et de '\n' */
    char reqs[2000];
    memset(reqs, 0, sizeof(reqs));

    /*Linux interprete \r\n comme un seul saut de ligne, il ignore \r*/

    *t_to_join = 0;

    if(read(req->soc_com, reqs, sizeof(reqs)) < 0){
        perror("Erreur de lecture de la socket\n");
        /*return errno;*/
    }

    while(cpt < sizeof(reqs)){
        if((nb_CRLF % 2 == 0) && reqs[cpt] == '\r')
            nb_CRLF++;
        else if((nb_CRLF % 2 != 0) && reqs[cpt] == '\n')
            nb_CRLF++;
        else if(nb_CRLF != 0)
            nb_CRLF = 0;

        if(nb_CRLF == 4){
            req_tmp->msg[j] = reqs[cpt]; /* Pour recuperer le dernier \n*/
            nb_req++;
            nb_CRLF = 0;
            j=0;
            req_tmp->thread_to_join = *t_to_join;
            req_tmp->soc_com = req->soc_com;
            req_tmp->sin = req->sin;
            if (pthread_create(t_to_join, NULL, routine_answer, (void*)req_tmp) != 0) {
              printf("pthread_create\n");
              exit(1);
            }

            tab_tid[i] = *t_to_join;
            size_tab++;
            printf("Requete No. %d : %s\n", nb_req, req_tmp->msg);
            /*memset(req_tmp, 0, sizeof(req_tmp));*/

            req_tmp = malloc(sizeof(struct requete));
        }
        else{
            req_tmp->msg[j] = reqs[cpt];
            j++;
        }
        cpt++;
    }

    for(i = 0; i < size_tab; i++){
        pthread_join(tab_tid[i], NULL);
    }

    shutdown(req->soc_com, SHUT_WR | SHUT_RD);
    close(req->soc_com);

    free(req);
    free(req_tmp); /*On en alloue un en trop a la fin de la boucle que l'on free ici*/
    free(t_to_join);
    pthread_exit(NULL);
}

/*ERRNO IS THREAD-SAFE
  errno is thread-local; setting it in one thread does not affect its value in any other thread.*/
void *routine_answer(void* arg) {
  requete *req = (requete*)arg;
  int fd, retour_read;
  int erreur = 0; /*s'il y a erreur alors egal a 1*/
  char download_buffer[BUFFERSIZE];
  char http_header[50];
  char filepath_cpy[50];
  char http_code[5] = "200 ";
  char http_msg_retour[50] = "OK\n";
  char *filepath, *extension, *type;
  struct stat stat_info;

  if(req->thread_to_join != 0){
    pthread_join(req->thread_to_join, NULL);
  }

  filepath = getFilepath(req->msg);
  strcpy(filepath_cpy, filepath);
  extension = getExtension(filepath_cpy);
  type = getType(extension);

  if ( stat(filepath, &stat_info) == -1) {
      perror("Erreur stat dans routine_answer\n");
      /*return errno;*/
  }

  if(S_ISDIR(stat_info.st_mode)){
      erreur = 1;
      strcpy(http_code, "400 ");
      strcpy(http_msg_retour, "Bad Request\n");
  }
  else if((fd = open(filepath, O_RDWR, 0644)) == -1){
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
  free(req);

  pthread_exit(NULL);
}
