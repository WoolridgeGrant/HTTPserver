#ifndef STRUCT_H
#define STRUCT_H
#define BUFFERSIZE 200

typedef struct mime mime;
struct mime{
	char type[100];
	char ext[20];
};

#endif
