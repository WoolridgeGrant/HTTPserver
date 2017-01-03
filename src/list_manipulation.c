#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

#include "../include/globals.h"
#include "../include/list_manipulation.h"

/*TODO : 1. check si seuil non atteint
		2. timer & delete*/

void add_elem_ip(elem *element){
	elem *element_tmp;
	if(liste_ip.first == NULL){
		element->prev = NULL;
		element->next = NULL;
		liste_ip.first = element;
		liste_ip.last = element;
	}
	else{
		element_tmp = liste_ip.first;
		do{
			if(strcmp(element_tmp->info.ip, element->info.ip) == 0){
				element_tmp->info.data += element->info.data;
				free(element);
				return;
			}
		}while((element_tmp = element_tmp->next) != NULL);
		/*Le cas ou l'adresse ip de l'element a inserer n'est pas encore dans la liste*/
		element->prev = liste_ip.last;
		element->next = NULL;
		liste_ip.last->next = element;
		liste_ip.last = element;
	}
}

void add_elem_req(elem *element){ /*free d'element au moment de delete*/
	if(liste_req.first == NULL){
		element->prev = NULL;
		element->next = NULL;
		liste_req.first = element;
		liste_req.last = element;
	}
	else{
		/*On ajoute quoiqu'il arrive car on doit decrementer data dans 60 secs*/
		element->prev = liste_req.last;
		element->next = NULL;
		liste_req.last->next = element;
		liste_req.last = element;
	}
}

void print_list(){
	elem *element;
	if(liste_ip.first != NULL){
		element = liste_ip.first;
		printf("Liste ip\n");
		do{
			printf("Data : %d\n", element->info.data);
		}while( (element = element->next) != NULL);
	}
	else{
		printf("Liste ip vide\n");
	}
	if(liste_req.first != NULL){
		element = liste_req.first;
		printf("Liste req\n");
		do{
			printf("Data : %d\n", element->info.data);
			printf("Thread en charge : %ld\n", element->info.tid);
		}while( (element = element->next) != NULL);
	}
	else{
		printf("Liste req vide\n");
	}
}

void delete_elem_ip(elem *element){


}


void delete_elem_req(elem *element){


}

/*check par routine_answer
sem_wait(1);
sem_wait(2);
cpt++;
sem_post(1);
sem_post(2);
pthread_mutex_lock de la case
Check l'autorisation
Si timer > 0 alors remettre timer direct a 10 et retourner 403
pthread_mutex_unlock de la case
cpt--;
*/

/*Add + mutex initialiser par routine_watcher OK
sem_wait(2);
while(cpt!=0){}
Traitement
sem_post(2);
*/

/*Parcours et decrement de tous les timer par routine_clock_list_ip
sem_wait(1);
sem_wait(2);
cpt++;
sem_post(1);
sem_post(2);
Decrementer/boucle
si == 0
	-> faire delete
cpt--;
sleep(1)*/

/* Delete par routine_clock
sem_wait(2);
while(cpt!=0){}
Traitement
sem_post(2);
*/

/*Parcours de list_req et decrement de tous les data et timer par routine_clock_list_req
sem_wait(1);
sem_wait(2);
cpt++;
sem_post(1);
sem_post(2);
Decrementer/boucle
si == 0
	parcourir la liste jusqua tomber sur l'ip
	pthread_mutex_lock de la case
	-> Decrementer data dans la case
	pthread_mutex_unlock
cpt--;*/

/*Update/Increment data par routine_answer tout a la fin avant les log OK, finalement dans routine_watcher
sem_wait(1);
sem_wait(2);
cpt++;
sem_post(1);
sem_post(2);
pthread_mutex_lock de la case
Traitement
pthread_mutex_unlock de la case
cpt--;
*/
