/*
生产者线程负责接收连接
消费者负责接收请求并处理
**/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include <semaphore.h>  
#include <pthread.h>  
#include "svrdata.h"

#define MSGKEY 1024  

#define BACKLOG   5
#define BUFSIZE   128 

struct sockqueue *msg = NULL;
int portnumber;

int main(int argc, char* argv[])
{
	int res,i;
  if ( 2 == argc )
  {
      if( (portnumber = atoi(argv[1])) < 0 )
      {
          fprintf(stderr,"Usage:%s portnumber/a/n",argv[0]);
          return 1;
      }
  }
  else
  {
      fprintf(stderr,"Usage:%s portnumber/a/n",argv[0]);
      return 1;
  }
  
  pthread_t thread_p,thread_c;
	void *thread_result = NULL; 

	msg = (struct sockqueue*)malloc(sizeof(struct sockqueue));
  //初始化结构体
  msg->ridx = 0;
  msg->ridx = 0;
  msg->count = 0;
  for(i=0;i<MSG_NUM;i++)
  	msg->Qsock[i]=0;

   /********设置队列操作信号量********/ 
   
    //初始化信号量,初始值为0  
    res = sem_init(&msg->rsem, 0, 0);  
    if(res == -1)  
    {  
        perror("semaphore intitialization failed\n");  
        exit(EXIT_FAILURE);  
    }  
    //初始化信号量,初始值为1 
    res = sem_init(&msg->wsem, 0, MSG_NUM);  
    if( res == -1)  
    {  
        perror("semaphore intitialization failed\n");  
        exit(EXIT_FAILURE);  
    }
    
    /********设置队列操作信号量********/ 

     //在主线程中屏蔽SIGINT信号，以防被子线程继承
    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);   
    //创建生产者线程，并把msg作为线程函数的参数  
    res = pthread_create(&thread_p, NULL, thread_producer, NULL);  
    if(res != 0)  
    {
        perror("producer thread_create failed\n");  
        exit(EXIT_FAILURE);  
    }
    //创建消费者线程，并把msg作为线程函数的参数  
    res = pthread_create(&thread_c, NULL, thread_consumer, NULL);  
    if(res != 0)  
    {
        perror("consumer thread_create failed\n");  
        exit(EXIT_FAILURE);  
    }	
    
    //恢复主线程信号
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    
    //创建信号处理
    struct sigaction s;
    s.sa_handler = sigchld_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);    

    //等待信号  

  pause();    
  //结束子线程		
	res = pthread_cancel(thread_p);    
  if(res != 0)  
  {  
      perror("pthread_cancel failed\n");  
      exit(EXIT_FAILURE);  
  }    
	res = pthread_cancel(thread_c);    
  if(res != 0)  
  {  
      perror("pthread_cancel failed\n");  
      exit(EXIT_FAILURE);  
  }  
  //等待子线程结束    
  res = pthread_join(thread_p, &thread_result);  
  if(res != 0)  
  {  
      perror("pthread_join failed\n");  
      exit(EXIT_FAILURE);  
  }
  printf("Thread joined\n");  
  //清理信号量  
  sem_destroy(&msg->rsem);  
  sem_destroy(&msg->wsem); 
  exit(EXIT_SUCCESS); 

	return 0; 
}

void* thread_producer(void *msgs)
{
	int rcd;
  int server_sockfd,new_cli_fd;
  socklen_t clisocklen,server_len;
  server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sockfd == -1) {
		printf("create server_socket error!\n");
		exit(1);
	}
	//设为非阻塞
	if (fcntl(server_sockfd, F_SETFL, O_NONBLOCK) == -1) {
		printf("Set server socket nonblock failed\n");
		exit(1);
	}
	
	sem_wait(&msg->wsem);
	struct sockaddr_in server_sockaddr,cli_sockaddr;
	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//设置监听端口
	server_sockaddr.sin_port = htons(portnumber);
	server_len = sizeof(server_sockaddr);	
	//绑定
	rcd = bind(server_sockfd, (struct sockaddr *) &server_sockaddr, server_len);
	if (rcd == -1) {
		printf("bind port %d error!\n", ntohs(server_sockaddr.sin_port));
		exit(1);
	}
	//监听
	rcd = listen(server_sockfd, BACKLOG);
	if (rcd == -1) {
		printf("listen error!\n");
		exit(1);
	}	
	printf("Server is  waiting on socket=%d \n", server_sockfd);
	
//	int watch_fd_list[BACKLOG + 1];
	fd_set watchset;
	struct timeval tv;
//	for (ci = 0; ci <= BACKLOG; ci++)
//		watch_fd_list[ci] = -1;
	FD_ZERO(&watchset);
	FD_SET(server_sockfd, &watchset);	
	
	while (1)	
	{
		tv.tv_sec = 20;
		tv.tv_usec = 0;
		FD_ZERO(&watchset);
		FD_SET(server_sockfd, &watchset);	
		rcd = select(server_sockfd + 1, &watchset, NULL, NULL, &tv);
		switch (rcd)
		{
			case -1:
				printf("Select error\n");
				exit(1);
			case 0:
				printf("Select time_out\n");
				FD_ZERO(&watchset);
				FD_CLR(server_sockfd, &watchset);
				FD_SET(server_sockfd, &watchset);
				break;
			default:
				if (FD_ISSET(server_sockfd, &watchset))
				{
					clisocklen = sizeof(cli_sockaddr);
					new_cli_fd = accept(server_sockfd,(struct sockaddr *) &cli_sockaddr, &clisocklen);
					if (new_cli_fd < 0)
					{
						printf("Accept error\n");
						exit(1);
					}
					printf("\nopen communication with  Client %s on socket %d\n",inet_ntoa(cli_sockaddr.sin_addr), new_cli_fd);
					
					datapull(new_cli_fd);
					sem_post(&msg->rsem);
					sem_wait(&msg->wsem);  

				}
				FD_ZERO(&watchset);
				FD_CLR(server_sockfd, &watchset);
				FD_SET(server_sockfd, &watchset);				
		}
	}
	pthread_exit(NULL);	
}
void *thread_consumer(void * str)
{
	int i = 0,j=0;  
    char buf[BUF_SIZE]={0};
    int ret,cli_sock,n;

    while(1)  
    {
    	sem_wait(&msg->rsem); 
    	ret = dataget(&cli_sock);
      if(ret)
      	continue;
      	
      sem_post(&msg->wsem);

    	printf("waitting msg from socket %d \n",cli_sock);
    	while(1)
    	{
    		memset(buf,0x00,sizeof(buf));
	    	n = recv(cli_sock, buf, BUF_SIZE-1,0);
	    	if(n > 0)
	    	{
	    		printf("received msg: [%s]\n",buf);
	    		write(cli_sock, buf, n);
	    	}
	    	if((n <= 0 && errno != EAGAIN)||(LINKMODE))//1 短连接 0长连接
	    	{
	    		shutdown(cli_sock,2);
	    		printf("socket %d closed \n",cli_sock);
	    		break;
	    	}
    	}
//	        for(i=0;i<2;i++)
//	        	for(j=0;j<100000000;j++);
    }

    //退出线程  
    pthread_exit(NULL);  
}

/**************************
队列操作函数
***************************/
int dataget(int *sockfd)
{
	if(msg->count<=0)
	{
		printf("Queue is empty\n");
		return 1;
	}
	else
	{
		msg->count--;
		if(msg->ridx >= MSG_NUM)
			msg->ridx = 0;
	}
	*sockfd = msg->Qsock[msg->ridx];
	msg->Qsock[msg->ridx] = -1;
	msg->ridx++;
	printf("pop:%d,idx:%d\n",*sockfd,msg->ridx-1);
	return 0;
}

int datapull(int sockfd)
{
	
	if(msg->count>=MSG_NUM)
	{
		printf("Queue is full\n");
		return 1;
	}
	else
	{
		msg->count++;
		if(msg->widx >= MSG_NUM)
			msg->widx = 0;
	}
	msg->Qsock[msg->widx]=sockfd;
	msg->widx++;
	printf("push:%d,idx:%d\n",sockfd,msg->widx-1);
	return 0;
}
/**************************
进程终止信号回调函数
***************************/
void sigchld_handler( int signo ){
	printf("sigchld_handler\n");
	if (signo == SIGINT) 
	{ 
		printf("get SIGINT\n"); 
  }
    return;
}
