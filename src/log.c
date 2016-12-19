#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/struct.h"
#include "../include/globals.h"
#include "../include/log.h"

/*Creer tmp*/
void addLog(requete *req){
  char log[500], size[50], pid[50], tid[20], ip[20];
  char n[2] = "\n";
  time_t rawtime;
  struct tm *timeinfo;
  int i;

  printf("\n**** log ****\n\n");

  memset(log, 0, sizeof(log));

  /*msg*/
  i = 0;
  while(req->msg[i] != '\n'  && req->msg[i] != '\r'){ /*Appelez moi detective Conan : C'est une sequence CRLF (\r suivi de \n) qui separe les lignes*/
    log[i] = req->msg[i];
    i++;
  }

  /*size file*/
  printf("size_file : %d\n", req->size_file);
  strcat(log, ", size : ");
  sprintf(size, "%d", req->size_file);
  strcat(log, size);


  /*http_code*/
  printf("http_code : %s\n", req->http_code);
  strcat(log, ", http_code : ");
  strcat(log, req->http_code);


  /*ip clt*/
  printf("ip : %s\n", inet_ntoa(req->sin.sin_addr));
  strcat(log, ", ip : ");
  sprintf(ip, "%s", inet_ntoa(req->sin.sin_addr));
  strcat(log, ip);


  /*pid server*/
  printf("pid server : %d\n", getpid());
  strcat(log, ", PID server : ");
  sprintf(pid, "%d", getpid());
  strcat(log, pid);


  /*tid*/
  printf("tid : %ld\n", (long)pthread_self());
  strcat(log, ", tid : ");
  sprintf(tid, "%ld", (long)pthread_self());
  strcat(log, tid);


  /*date et heure*/
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  strcat(log, ", time : ");
  strcat(log, asctime (timeinfo));
  printf("date : %s\n", asctime (timeinfo));


  /*ecrire dans le fichier de log*/

  pthread_mutex_lock(&fd_log_mutex);
  i = 0;
  while(log[i] != '\0'){
    if(log[i] != '\n' && write(fd_log, &log[i], sizeof(char)) < 0){
      perror("Erreur d'ecriture dans le fichier de log\n");
      /*return errno;*/
    }
    i++;
  }

  if(write(fd_log, &n, sizeof(char)) < 0){
    perror("Erreur d'ecriture dans le fichier de log\n");
    /*return errno;*/
  }


  pthread_mutex_unlock(&fd_log_mutex);

  printf("\n-> log ajouté\n");
}
