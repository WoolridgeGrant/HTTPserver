#include "struct.h" /*TODO : A revoir surement pas la maniere optimale de faire ca*/
#include "mqueue.h"

extern mime mimes[800];
extern int size_mimes;
extern int fd_log;
extern pthread_mutex_t fd_log_mutex;
extern char filename_log[30];

extern mqd_t mq_des;
extern struct mq_attr attr;
