#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include "struct.h"

#define PORT 4567
#define LISTEN_QUEUE 10
//#define MAXLEN_CONTENT 1024
#define MSG_STORE_LIMIT 4

int main()
{
	int listenfd,newfd;
	struct sockaddr_in address;
	int opt=1;
	int ppid,pid;
	message *msg_map;
	int i=0;
	int *from;  

	char buf[MAXLEN_CONTENT];

	//PTHREAD_MUTEX_INITIALIZER静态初始化互斥锁
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);  //初始化互斥对象属性     
    pthread_mutexattr_setpshared(&mutexattr,PTHREAD_PROCESS_SHARED);  //设置互斥对象为

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

	 msg_map=(message*)mmap(NULL,sizeof(message)*1024,PROT_READ|PROT_WRITE,
       MAP_SHARED|MAP_ANONYMOUS,-1,0);
	
    from=(int *)mmap(NULL,sizeof(int),PROT_READ|PROT_WRITE,
       MAP_SHARED|MAP_ANONYMOUS,-1,0);
    (*from)=0;

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
			 //因为进程的复制，会有很多指向listenfd，我们只剩一个即可
			close(listenfd); 
			int content_bytes;
		pthread_mutex_lock(&mutex); 
		(*from)++;
		pthread_mutex_unlock(&mutex);

		printf("%d\n",*from);
		int my_client ;
		my_client = *from;  //标记客户
		printf("myclient %d\n",my_client);
		pid=fork();
		if(pid==0)
		{
			//printf("pid my_client %d\n",my_client);
			while(1)
			{

				if((content_bytes=recv(newfd,(void *)buf,MAXLEN_CONTENT,0))==-1)
				{
					perror("receive error!\n");
					close(newfd);
					exit(1);
				}

				if(content_bytes>0)
				{
					//printf("revc %s",buf);
					buf[content_bytes]='\0';
											
					//printf("save message\n");
					pthread_mutex_lock(&mutex);
					msg_map[0].from++;
					if(msg_map[0].from >= MSG_STORE_LIMIT) 
					{
						msg_map[0].from=(msg_map[0].from%MSG_STORE_LIMIT)+1;
					}					
					i=msg_map[0].from;
					msg_map[0].id++;
					strcpy( msg_map[i].content,buf);
					msg_map[i].id=msg_map[0].id;
					msg_map[i].len = strlen(buf);
					msg_map[i].from=my_client;
					pthread_mutex_unlock(&mutex);										
				}
			}					
		}
		if(pid>0)
		{
			i=0;
			int last_send_msg_id = 0;
			if(msg_map[0].id >= MSG_STORE_LIMIT)
			{
				int t;
				t=msg_map[0].from;
				if( msg_map[t].id > last_send_msg_id)
				{
					i=t;
					last_send_msg_id=msg_map[t+1].id-1;
				}
			}
			//printf("pid p %d\n",my_client);
			while(1)
			{
				if( msg_map[0].id >last_send_msg_id )
				{
					last_send_msg_id++;
					i++;
					if(i>=MSG_STORE_LIMIT)
						i=(i%MSG_STORE_LIMIT)+1;
					//printf("pid my_client:%d  from: %d\n",my_client,msg_map[i].from);
					if(msg_map[i].from != my_client)
					{
						printf("send %d %s:%d\n",last_send_msg_id,msg_map[i].content,msg_map[i].id);
						strcpy(buf,msg_map[i].content);
						if((content_bytes=send(newfd,(void *)buf,strlen(buf),0))==-1)
						{
							perror("send error!\n");
							close(newfd);
							exit(1);
						}						
					}					
				}	
			}
		}
			close(newfd);
		}

		close(newfd);

	}
	close(listenfd);
	return 0;
}
