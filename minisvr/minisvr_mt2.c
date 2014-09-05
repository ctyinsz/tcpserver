#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#define BACKLOG   2
#define BUFSIZE   128

int u_open(int port)
{
	int rcd;
	int  server_len;
	int act = 1;
	int server_sockfd;
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sockfd == -1) {
		printf("create server_socket error!\n");
		return -1;
	}

	/*��ַ����*/
	if ( setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&act,sizeof(act))==-1)
  {
    while( (close(server_sockfd) == -1 ) && (errno==EINTR));
    printf("errno=[%d][%s]\n",errno,strerror(errno));
  }
  /*�˿�����*/
  #ifdef  SO_REUSEPORT
  if ( setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEPORT,&act,sizeof(act))==-1)
  {
    while( (close(server_sockfd) == -1 ) && (errno==EINTR));
    printf("errno=[%d][%s]\n",errno,strerror(errno));
  }
  #endif
	//��Ϊ������
	if (fcntl(server_sockfd, F_SETFL, O_NONBLOCK) == -1) {
		printf("Set server socket nonblock failed\n");
		return -1;
	}	
	struct sockaddr_in server_sockaddr;
	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//���ü����˿�
	server_sockaddr.sin_port = htons(port);
	server_len = sizeof(server_sockaddr);
	//��
	rcd = bind(server_sockfd, (struct sockaddr *) &server_sockaddr, server_len);
	if (rcd == -1) {
		printf("bind port %d error!\n", ntohs(server_sockaddr.sin_port));
		return -1;
	}
	//����
	rcd = listen(server_sockfd, BACKLOG);
	if (rcd == -1) {
		printf("listen error!\n");
		return -1;
	}
	printf("Server is  waiting on socket=%d \n", server_sockfd);
	return 	server_sockfd;
}

int dealreq(int sock)
{
	int rcd;
	fd_set rset;
	struct timeval rtv;
	struct sockaddr_in sockaddr;
	char addrip[20]={0};
	char buffer[BUFSIZE+1]={0};
	int clisocklen = sizeof(sockaddr);
	if(sock<=0)
		exit(-1);
		
	//��ÿͻ��˵�IP��ַ
	getpeername(sock, (struct sockaddr*) &sockaddr,&clisocklen);
	sprintf(addrip,"%s",inet_ntoa(sockaddr.sin_addr));
	FD_ZERO(&rset);
	while(sock>0)
	{
		memset(buffer,0,sizeof(buffer));
		FD_SET(sock, &rset);
		rtv.tv_sec = 20;
		rtv.tv_usec = 0;
		rcd = select(sock + 1, &rset, NULL, NULL, &rtv);
		switch (rcd)
		{
			case -1:
				printf("Select error\n");
				break;
			case 0:
				printf("Select time_out\n");
				break;
			default:
				if (FD_ISSET(sock, &rset)) 
				{
					int len = recv(sock, buffer, BUFSIZE, 0);
					if((len <= 0)&&(errno != EAGAIN))
					{
						shutdown(sock,2);
						FD_CLR(sock, &rset);
						printf("\nCommunication with  Client %s on socket %d Closed\n",addrip, sock);
						exit(0);
					}
					printf("read data [%s] from Client %s on socket %d\n",buffer,addrip,sock);
					//���ͽ��յ�������
					len = send(sock, buffer, strlen(buffer), 0);
					if (len < 0)
					{
						printf("Send error\n");
						exit(-1);	
					}
					printf("write data [%s] to Client %s on socket %d\n",buffer, addrip,sock);
					//���յ����ǹر�����
					if (strcmp(buffer, "quit") == 0) 
					{
						shutdown(sock,2);
						printf("Connection on socket[%d] closed by client[%s]!\n",sock,addrip);
						sock = -1;
						exit(0);
					}								
				}
				break;
		}
	}
	exit(0);
}

void select_test(int port) {
	int rcd;
	int new_cli_fd;
	int clisocklen,socklen;
	int ci;

	int server_sockfd;
	server_sockfd = u_open(port);

	//��ʼ����������
	fd_set watchset;
	struct timeval tv; /* ����һ��ʱ�����������ʱ�� */
	struct sockaddr_in cli_sockaddr;
	
	

	while(1)
	{
		tv.tv_sec = 20;
		tv.tv_usec = 0;/* ����select�ȴ������ʱ��Ϊ20��*/
		FD_ZERO(&watchset);
		FD_SET(server_sockfd, &watchset);
		rcd = select(server_sockfd + 1, &watchset, NULL, NULL, &tv);
		switch (rcd)
		{
			case -1:
				printf("Select error\n");
				break;
			case 0:
//				printf("Select time_out\n");
				break;
			default:
				if (FD_ISSET(server_sockfd, &watchset))
				{
					socklen = sizeof(cli_sockaddr);
					new_cli_fd = accept(server_sockfd,(struct sockaddr *) &cli_sockaddr, &socklen);
					if (new_cli_fd < 0)
					{
						printf("Accept error\n");
						exit(1);
					}
					printf("\nopen communication with  Client %s on socket %d\n",inet_ntoa(cli_sockaddr.sin_addr), new_cli_fd);
					switch (fork())
					{
						case -1:
							printf("fork error\n");
							continue;
						case 0:
							dealreq(new_cli_fd);
							break;
						default:
							break;
					}
				} 
				break;
		}
	}
	printf("\nThis is main proc!\n");
}

int main(int argc, char* argv[])
{
	int portnumber;
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
    signal( SIGCHLD, SIG_IGN );
    select_test(portnumber);
    return 0; 
}

