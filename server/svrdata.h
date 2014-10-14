#ifndef _SVRDATA_H_HEADER
#define _SVRDATA_H_HEADER

#define BUF_SIZE 512
#define MSG_NUM 3  
#define LINKMODE 0 //1 短连接 0长连接
char SEM_NAME[]= "cty";

struct sockqueue
{
//	volatile 	int written;//作为一个标志，非0：表示可读，0表示可写
//	char smsg[MSG_NUM][BUF_SIZE];//记录写入和读取的文本
	int Qsock[MSG_NUM];
	int ridx;
	int widx;
	int count;
	sem_t rsem;
	sem_t wsem;
//	struct timeval tv;
};

#endif

void *thread_producer(void *);
void *thread_consumer(void *); 
int dataget(int *sockfd);
int datapull(int sockfd);
void sigchld_handler( int signo );
