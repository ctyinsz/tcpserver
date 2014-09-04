#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <sys/fcntl.h>
#include <errno.h>

#define BACKLOG   5
#define BUFSIZE   128

void select_test(int port, int backlog) {
	int rcd;
	int new_cli_fd;
	int maxfd;
	int socklen, server_len,clisocklen;
	int ci;
	int watch_fd_list[backlog + 1];
	for (ci = 0; ci <= backlog; ci++)
		watch_fd_list[ci] = -1;

	int server_sockfd;
	//建立socket，类型为TCP流
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

	struct sockaddr_in server_sockaddr;
	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//设置监听端口
	server_sockaddr.sin_port = htons(port);
	server_len = sizeof(server_sockaddr);
	//绑定
	rcd = bind(server_sockfd, (struct sockaddr *) &server_sockaddr, server_len);
	if (rcd == -1) {
		printf("bind port %d error!\n", ntohs(server_sockaddr.sin_port));
		exit(1);
	}
	//监听
	rcd = listen(server_sockfd, backlog);
	if (rcd == -1) {
		printf("listen error!\n");
		exit(1);
	}
	printf("Server is  waiting on socket=%d \n", server_sockfd);

	watch_fd_list[0] = server_sockfd;
	maxfd = server_sockfd;

	//初始化监听集合
	fd_set watchset;
	FD_ZERO(&watchset);
	FD_SET(server_sockfd, &watchset);

	struct timeval tv; /* 声明一个时间变量来保存时间 */
	struct sockaddr_in cli_sockaddr;
	while (1) {

		tv.tv_sec = 20;
		tv.tv_usec = 0; /* 设置select等待的最大时间为20秒*/
		//每次都要重新设置集合才能激发事件
		FD_ZERO(&watchset);
		FD_SET(server_sockfd, &watchset);
		//对已存在到socket重新设置
		for (ci = 0; ci <= backlog; ci++)
			if (watch_fd_list[ci] != -1) {
				FD_SET(watch_fd_list[ci], &watchset);
			}

		rcd = select(maxfd + 1, &watchset, NULL, NULL, &tv);
		switch (rcd) {
		case -1:
			printf("Select error\n");
			exit(1);
		case 0:
			printf("Select time_out\n");
			//超时则清理掉所有集合元素并关闭所有与客户端的socket
			FD_ZERO(&watchset);
			for (ci = 1; ci <= backlog; ci++){
				shutdown(watch_fd_list[ci],2);
				watch_fd_list[ci] = -1;
			}
			//重新设置监听socket，等待链接
			FD_CLR(server_sockfd, &watchset);
			FD_SET(server_sockfd, &watchset);
			continue;
		default:
			//检测是否有新连接建立
			if (FD_ISSET(server_sockfd, &watchset)) { //new connection
				socklen = sizeof(cli_sockaddr);
				new_cli_fd = accept(server_sockfd,
						(struct sockaddr *) &cli_sockaddr, &socklen);
				if (new_cli_fd < 0) {
					printf("Accept error\n");
					exit(1);
				}
				printf("\nopen communication with  Client %s on socket %d\n",
						inet_ntoa(cli_sockaddr.sin_addr), new_cli_fd);

				for (ci = 1; ci <= backlog; ci++) {
					if (watch_fd_list[ci] == -1) {
						watch_fd_list[ci] = new_cli_fd;
						break;
					}
				}
				if((ci == backlog+1)&&(watch_fd_list[ci] != new_cli_fd))
				{
					printf("Server is busy\n");
					close(new_cli_fd);
					continue;
				}

				FD_SET(new_cli_fd, &watchset);
				if (maxfd < new_cli_fd) {
					maxfd = new_cli_fd;
				}

				continue;
			} else {//已有连接的数据通信
				//遍历每个设置过的集合元素
				for (ci = 1; ci <= backlog; ci++) { //data
					if (watch_fd_list[ci] == -1)
						continue;
					if (!FD_ISSET(watch_fd_list[ci], &watchset)) {
						continue;
					}
					char buffer[BUFSIZE+1]={0};
					//接收
					int len = recv(watch_fd_list[ci], buffer, BUFSIZE, 0);
					if((len <= 0)&&(errno != EAGAIN))
					{
							shutdown(watch_fd_list[ci],2);
							FD_CLR(watch_fd_list[ci], &watchset);
							watch_fd_list[ci] = -1;
							printf("\ncommunication with  Client %s on socket %d Closed\n",inet_ntoa(cli_sockaddr.sin_addr), new_cli_fd);
							continue;
					}
//					buffer[len+1] = 0;

					//获得客户端的IP地址
					struct sockaddr_in sockaddr;
					char addrip[20]={0};
					getpeername(watch_fd_list[ci], (struct sockaddr*) &sockaddr,
							&clisocklen);
					sprintf(addrip,"%s",inet_ntoa(sockaddr.sin_addr));

					printf("read data [%s] from Client %s on socket %d\n",
							buffer,addrip,watch_fd_list[ci]);
					//发送接收到到数据
					len = send(watch_fd_list[ci], buffer, strlen(buffer), 0);
					if (len < 0) {
						printf("Send error\n");
						exit(1);
					}
					printf("write data [%s] to Client %s on socket %d\n",
							buffer, addrip,watch_fd_list[ci]);

//					shutdown(watch_fd_list[ci],2);
//					watch_fd_list[ci] = -1;
//					FD_CLR(watch_fd_list[ci], &watchset);
//					watch_fd_list[ci] = -1;

					//接收到的是关闭命令
					if (strcmp(buffer, "quit") == 0) {
						for (ci = 0; ci <= backlog; ci++)
							if (watch_fd_list[ci] != -1) {
								shutdown(watch_fd_list[ci],2);
							}
						printf("\nWeb Server Quit!\n");
						exit(0);
					}
				}
			}
			break;
		}
	}
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
    
    select_test(portnumber,BACKLOG);
    return 0; 
}

