#ifndef STRUCT_H
#define STRUCT_H
#define BUFFERSIZE 200

#include <netinet/in.h>

typedef struct mime mime;
struct mime{
	char type[100];
	char ext[20];
};

typedef struct requete requete;
struct requete{
	struct sockaddr_in sin;
	char msg[2000];
	char http_code[10];
	int soc_com;
	int size_file;
};

#endif
