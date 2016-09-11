#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "struct.h"

int main(int argc,char **argv)
{
    int sockfd;
    struct sockaddr_in address;
    int last_revc_msg_id=0;  //an id of the last message client receive
    int pid;

    char buf[MAXLEN_CONTENT];
    int content_bytes;

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

    pid=fork();
    if(pid==-1)
    {
        perror("fork error!\n");
        close(sockfd);
        exit(1);
    }
    if(pid==0)
    {
        while(1)
        {
            fgets(buf,MAXLEN_CONTENT,stdin);
            //printf("send: %s",buf);
            if(send(sockfd,(void *)&buf,strlen(buf),0)==-1)
            {
                perror("send error!\n");
                close(sockfd);
                exit(1);
            }
        }
    }
    if(pid>0)
    {          
        while(1)
        {
            memset(buf,0,MAXLEN_CONTENT);
            if((content_bytes=recv(sockfd,(void *)&buf,MAXLEN_CONTENT,0))==-1)
            {
                    perror("revc error!\n");
                    close(sockfd);
                    exit(1);
            }
            if(content_bytes>0)
            {
                    //printf("recv message!\n");
                    fputs(buf,stdout);
            }
        }
    }       

    close(sockfd);
    return 0;
}
