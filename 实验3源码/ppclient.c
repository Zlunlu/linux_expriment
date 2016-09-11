#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXLEN_CONTENT 1024

int main(int argc,char **argv)
{
    if(argc<3)
    {
        printf("please input: %s host port\n",argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in address;

    char buf[MAXLEN_CONTENT];

    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        perror("create socket error!\n");
        exit(1);
    }

    address.sin_family=AF_INET;
    address.sin_addr.s_addr=inet_addr(argv[1]);
    address.sin_port=htons(atoi(argv[2]));

    if(connect(sockfd,(struct sockaddr *)&address,sizeof(address))==-1)
    {
        perror("connect error!\n");
        exit(1);
    }

    while(1)
    {
        fgets(buf,MAXLEN_CONTENT,stdin);
        if(send(sockfd,(void *)buf,MAXLEN_CONTENT,0)==-1)
        {
            perror("send error!\n");
            close(sockfd);
            exit(1);
        }
        if(recv(sockfd,(void *)buf,MAXLEN_CONTENT,0)==-1)
        {
            perror("revc error!\n");
            close(sockfd);
            exit(1);
        }
        fputs(buf,stdout);
    }
    close(sockfd);
    return 0;
}
