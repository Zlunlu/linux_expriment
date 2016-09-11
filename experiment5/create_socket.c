#include "create_socket.h"
//#include "struct.h"
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

int ServerCreateSocket()
{
	int listenfd;
	struct sockaddr_in address;
	int opt=1;
	if((listenfd=socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("create listen socket error!\n");
		exit(1);
	}

	address.sin_family=AF_INET;
	address.sin_port=htons(PORT);
	address.sin_addr.s_addr=htonl(INADDR_ANY);

	//close socket还可以持续一段时间，减少因为突然关闭而造成数据的丢失
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

	return listenfd;
}

int clientCreateSsocket()
{
    int sockfd;
    struct sockaddr_in address;
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        perror("create socket error!\n");
        exit(1);
    }

    address.sin_family=AF_INET;
    address.sin_addr.s_addr=inet_addr("127.0.0.1");
    address.sin_port=htons(PORT);

    if(connect(sockfd,(struct sockaddr *)&address,sizeof(address))==-1)
    {
        perror("connect error!\n");
        exit(1);
    }

    return sockfd;
}
