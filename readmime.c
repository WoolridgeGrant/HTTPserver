#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


struct mime{
	char type[100];
	char ext[20];
};

struct mime mimes[800];
int size_mimes = 0;
char cnull[] = "null";

char* getType(char *ext){
	int nb = 0;
	int b = 0;
	char* c;
	while(b == 0 && nb <= size_mimes){
		if(strcmp(ext, mimes[nb].ext) == 0){
			b = 1;
		} else {
			nb++;
		}
	}

	if(b == 1){
		c = mimes[nb].type;
	} else {
		c = cnull;
	}

	return c;
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

int main(int argc, char* argv[]){
	int i;
	addTypes();

	for(i = 0; i < size_mimes; i++){
		printf("%s\t%s\n", mimes[i].type, mimes[i].ext);
	}

	char ext_search[] = "jar";
	char* res_type = getType(ext_search);
	printf("type de %s : %s\n", ext_search, res_type);

	return 0;
}
