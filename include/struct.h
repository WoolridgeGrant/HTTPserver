#ifndef STRUCT_H
#define STRUCT_H
#define PORTSERV 7100
#define BUFFERSIZE 200

typedef struct mime mime;
struct mime{
	char type[100];
	char ext[20];
};

#endif
