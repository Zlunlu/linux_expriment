#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 4567
#define LISTEN_QUEUE 6
#define MAXLEN_CONTENT 1024

int main()
{
	int listenfd,newfd;
	struct sockaddr_in address;
	int opt=1;

	char buf[MAXLEN_CONTENT];

	if((listenfd=socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("create listen socket error!\n");
		exit(1);
	}

	address.sin_family=AF_INET;
	address.sin_port=htons(PORT);
	address.sin_addr.s_addr=htonl(INADDR_ANY);

	if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(int))==-1)
	{
		perror("setsockopt error!\n");
		exit(1);
	}
	if(bind(listenfd,(struct sockaddr *)&address,sizeof(address))==-1)
	{
		perror("bind error!\n");
		exit(1);
	}
	if(listen(listenfd,LISTEN_QUEUE)==-1)
	{
		perror("listen error!\n");
	}

	while(1)
	{
		printf("wait server...\n");

		if((newfd=accept(listenfd,NULL,NULL))==-1)
		{
			perror("accpet error!\n");
			exit(1);
		}
		if(fork()==0)
		{
			close(listenfd);
			int content_bytes;
			while(1)
			{
				if((content_bytes=recv(newfd,(void *)buf,MAXLEN_CONTENT,0))==-1)
				{
					perror("receive error!\n");
					close(newfd);
					exit(1);
				}
				sleep(1);
				buf[content_bytes]='\0';
				if((content_bytes=send(newfd,(void *)buf,strlen(buf),0))==-1)
				{
					perror("send error!\n");
					close(newfd);
					exit(1);
				}
			}

			close(newfd);
		}

		close(newfd);
	}
	close(listenfd);
	return 0;
}
