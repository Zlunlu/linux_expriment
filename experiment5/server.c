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
#include <fcntl.h>
#include <signal.h>
#include "struct.h"
#include "create_socket.h"
#define PORT 4567  //通讯端口
#define LISTEN_QUEUE 6  //监听排队限制人数
#define MSG_STORE_LIMIT 2000  //服务器最大信息存储量
#define USER_LIMIT 1000  //人数限制，可增加

history_msg *msg_map;  //存储记录用的结构体指针
enroll *user_map;	//指向总的用户

//int ServerCreateSocket();
int readData();  //加载记录
void saveData(int signo);  //存储记录
int chatting(int newfd,char *my_client,char *client_name,int lmsg_id);  //进行聊天
int loginCheck(int newfd);  //处理登陆、注册、修改密码

int main()
{
	int listenfd,newfd;
	int ppid;
	
	int i=0;
	
	char buf[MAXLEN_CONTENT];	
	char client_name[NAME_LEN];  

	readData();
	listenfd=ServerCreateSocket();

    signal(SIGINT,saveData);  //捕捉Ctrl+C信号，做相应的处理

    //服务器的死循环，等待随时服务客户
	while(1)
	{
		printf("wait server...\n");

		if((newfd=accept(listenfd,NULL,NULL))==-1)
		{
			perror("accpet error!\n");
			exit(1);
		}

		//创建进程与客户保持连接
		if(fork()==0)
		{
			int client_location;
			int revc_bytes;
			int lmsg_id;
			pck umsg;

			signal(SIGINT,SIG_DFL);  //捕捉Ctrl+c信号，系统默认处理
			close(listenfd);  //因为进程复制，listenfd有多个指向，关掉其中一个

			client_location = loginCheck(newfd);


			//每个用户即将登录时会向服务器放发送上次接受最后到的信息的标号
			//此循环确保收到消息，否则断开连接
			do
			{
				if((revc_bytes=recv(newfd,&umsg,sizeof(umsg),0))==-1)
				{
					perror("recv error!");
				}
				if(revc_bytes>0)
				{
					if(umsg.type!=LOGIN)
					{
						printf("recv error message!\n");
						exit(1);
					}
					printf("%d\n",umsg.data.log.lmsg_id);
					lmsg_id = umsg.data.log.lmsg_id;
					break;
				}

			}while(1);
			
			char my_client[ACCOUNT_LEN];  //客户账号
			strcpy(my_client,user_map[client_location].account);
			strcpy(client_name,user_map[client_location].name);
			chatting(newfd,my_client,client_name,lmsg_id);			
		}
		
		close(newfd);
	}
	
	close(listenfd);
	return 0;
}

//save history message
void saveData(int signo)   
{
	printf("saveMsg!\n");
	int fd;
	int i;
	fd=open("chat_log",O_CREAT|O_WRONLY);
	write(fd,msg_map,MSG_STORE_LIMIT*sizeof(history_msg));
    close(fd);

    fd=open("user_list",O_CREAT|O_WRONLY);
	write(fd,user_map,USER_LIMIT*sizeof(enroll));
    close(fd);

	exit(0);
}

int readData()
{
	int fd;
	int i;

	fd=open("chat_log",O_CREAT|O_RDWR,00777);
	lseek(fd,sizeof(history_msg)*MSG_STORE_LIMIT-1,SEEK_SET);  //标号移动到文件的一个位置
    write(fd,"",1);  //确定文件头到这个位置的空间为文件大小

     //文件内容映射到共享内存，并通过强制转换，将其划分成多个history_msg
    //方便访问和写入
	msg_map =(history_msg*)mmap(NULL,sizeof(history_msg)*MSG_STORE_LIMIT,PROT_READ|PROT_WRITE,
       MAP_SHARED,fd,0); 
	
	close(fd);
	
	fd=open("user_list",O_CREAT|O_RDWR,00777);
	lseek(fd,sizeof(enroll)*USER_LIMIT-1,SEEK_SET);
    write(fd,"",1);
	user_map= (enroll*)mmap(NULL,sizeof(enroll)*USER_LIMIT,PROT_READ|PROT_WRITE,
       MAP_SHARED,fd,0);
	close(fd);
	
	return 0;
}
/*
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
*/

//功能: 实现聊天
//参数: newfd  一个新的socket描述符
//      *my_client  子进程服务的客户账号
//		client_name  子进程服务的客户名
//      lmsg_id(last message id)  客户上次收到的偶消息
//返回值: 正常结束则返回0
int chatting(int newfd,char *my_client,char *client_name,int lmsg_id)
{
	int i;
	char buf[MAXLEN_CONTENT];
	int content_bytes;
	chat_msg msg;
    
    //互斥变量设置
    //PTHREAD_MUTEX_INITIALIZER静态初始化互斥锁
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);  //初始化互斥对象属性     
    pthread_mutexattr_setpshared(&mutexattr,PTHREAD_PROCESS_SHARED);  //设置互斥对象为
    
	int pid=fork();
	
	//子进程接受信息，父进程发送消息，从而实现即时聊天
	if(pid==0)
	{

		signal(SIGINT,SIG_DFL);	
		while(1)
		{
			memset(buf,0,sizeof(buf));  //先清空，以免杂物影响
			if((content_bytes=recv(newfd,buf,MAXLEN_CONTENT,0))==-1)
			{
				perror("receive error!\n");
				close(newfd);
				exit(1);
			}
			if(content_bytes==0)
			{
				printf("client quit!\n");
				exit(0);
			}
			if(content_bytes>0)
			{
				//对共享内存进行写操作，实现读写互斥
				pthread_mutex_lock(&mutex); 
				msg_map[0].len++;  //最新记录的位置
				if(msg_map[0].len >= MSG_STORE_LIMIT)
					msg_map[0].len = msg_map[0].len % MSG_STORE_LIMIT +1;
				i = msg_map[0].len; //将最新记录的位置给i以便写入
				msg_map[0].id++;
				strcpy( msg_map[i].content,buf);
				msg_map[i].id=msg_map[0].id;
				msg_map[i].len = strlen(buf);
				strcpy(msg_map[i].from,my_client);
				strcpy(msg_map[i].from_name,client_name);
				pthread_mutex_unlock(&mutex);  //一定要解锁
			}
		}					
	}
	if(pid>0)
	{
		signal(SIGINT,SIG_DFL);
		i = lmsg_id;  //send the last message		
		if(msg_map[0].id >= MSG_STORE_LIMIT)
		{
			int t;  //临时变量
			t=msg_map[0].len;  //最新记录的位置
			t++;
			if(t == MSG_STORE_LIMIT)
				t=1;	
			if(msg_map[t].id > lmsg_id)
			{
				i = t-1;
				lmsg_id = msg_map[t].id-1;
			}
			if(msg_map[t].id <= lmsg_id)
			{
				t = (lmsg_id - msg_map[t].id)+t;
				if(t >= MSG_STORE_LIMIT)
					t = (t%MSG_STORE_LIMIT) + 1;
				i = t;
			}					
		}
		
		while(1)
		{
			if( msg_map[0].id > lmsg_id)
			{
				i++;
				lmsg_id++;
				if(i>=MSG_STORE_LIMIT)
						i=(i%MSG_STORE_LIMIT)+1;
				
				if(strcmp(msg_map[i].from,my_client)!=0)
				{
					printf("send %d\n",lmsg_id);
					printf("%d\n",msg_map[i].len);
					strcpy(msg.content,msg_map[i].content);
					strcpy(msg.name,msg_map[i].from_name);
					msg.id=msg_map[i].id;
					if((content_bytes=send(newfd,&msg,sizeof(chat_msg),0))==-1)
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
	return 0;
}


//功能: 登陆、注册、修改密码的处理
//参数: newfd: 新的socket用于与客户的通信
//返回值: 正常则返回0
int loginCheck(int newfd)
{
	pck umsg;
	int i;
	int ret;

	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);  //初始化互斥对象属性     
    pthread_mutexattr_setpshared(&mutexattr,PTHREAD_PROCESS_SHARED);  //设置互斥对象为
    
    //与客户订相同的协议
	enum flag { logined=1,unlogin=-1,enrolled=2,unenroll=-2,
		change_pws=3,unchange_pws=-3};
	do
	{
		memset(&umsg,0,sizeof(pck));
		if( (ret=recv(newfd,&umsg,sizeof(pck),0))==-1 )
		{
			printf("recv client information error!\n");
			exit(1);
		}
		if(ret == 0)
		{
			printf("client quit!\n");
			exit(0);
		}

		//登陆
		if(umsg.type==LOGIN)
		{
			printf("account:%s(%s) want to login\n",umsg.data.log.account,umsg.data.log.password);
			printf("user_list 0 account: %s(%s)\n",user_map[0].account,user_map[0].password);
			for(i=0;i<USER_LIMIT;i++)
			{				
				//先检验账号
				if(strcmp(user_map[i].account,umsg.data.log.account)==0)
				{
					printf("account exit\n");
					printf("password:%s want to login\n",user_map[i].password);
					//检验密码
					if(strcmp(user_map[i].password,umsg.data.log.password)==0)
					{
						//登陆成功的标志
						umsg.type = logined;
						printf("verify success!\n");
						break;
					}					
				}				
			}
			
			if(i==USER_LIMIT)
			{
				//登陆失败的标志
				umsg.type=unlogin;
				printf("verify error!\n");
			}

			//修改标志位后，将信息回送客户
			if( (ret=send(newfd,&umsg,sizeof(pck),0))==-1 )
			{
				printf("send client information error!\n");
				exit(1);
			}
			if(umsg.type==logined)
				return i;  //登陆成功就返回user_list的位置
		}

		//注册
		else if(umsg.type==ENROLL)
		{
			printf("account:%s(%s) want to enroll\n",umsg.data.log.account,umsg.data.log.password);
			for(i=0;i<USER_LIMIT;i++)
			{
				//检验该账号是否已被注册
				if(strcmp(user_map[i].account,umsg.data.enro.account)==0)
				{
					umsg.type = unenroll;
					printf("Have enrolled!\n");
					break;
				}
				if(strlen(user_map[i].account)==0)
				{
					printf("num %d client enrolled!\n",i);
					pthread_mutex_lock(&mutex); 
					user_map[i]=umsg.data.enro;
					pthread_mutex_unlock(&mutex); 
					umsg.type = enrolled;
					printf("account: %s password:%s \n",user_map[i].account,user_map[i].password);
					break;
				}
				
			}
			if(i==USER_LIMIT)
			{
				umsg.type=unenroll;
				printf("full! refuse enroll\n ");
				exit(1);
			}
			if( (ret=send(newfd,&umsg,sizeof(pck),0))==-1 )
			{
				printf("send client information error!\n");
				exit(1);
			}
		}

		//修改密码
		//收到客户发来的请求，检验客户信息正确后发给客户是否可以修改密码这个决定
		else if(umsg.type==CHANGE_PASSWORD)
		{
			printf("account:%s(%s) want to change password\n",umsg.data.log.account,umsg.data.log.password);
			printf("user_list 0 account: %s(%s)\n",user_map[0].account,user_map[0].password);
			
			//检验用户账号密码
			for(i=0;i<USER_LIMIT;i++)
			{
				if(strcmp(user_map[i].account,umsg.data.log.account)==0)
				{
					printf("account exit\n");
					printf("password:%s want to password\n",user_map[i].password);
					if(strcmp(user_map[i].password,umsg.data.log.password)==0)
					{
						umsg.type = change_pws;  //允许修改密码
						printf("verify success!\n");
						break;
					}					
				}				
			}
			if(i==USER_LIMIT)
			{
				umsg.type=unchange_pws;  //不允许修改密码
				printf("verify error!\n");

			}
			if( (ret=send(newfd,&umsg,sizeof(pck),0))==-1 )
			{
				printf("send client information error!\n");
				exit(1);
			}

			//不允许修改密码,下面修改密码的操作不能执行客户进行其他选择
			if(umsg.type==unchange_pws)
				continue;
			do
			{
				if( (ret=recv(newfd,&umsg,sizeof(pck),0))==-1 )
				{
					printf("recv client information error!\n");
					exit(1);
				}

				if(ret>0)
				{
					pthread_mutex_lock(&mutex);   //修改密码
					strcpy(user_map[i].password,umsg.data.change_pws.password);
					pthread_mutex_unlock(&mutex); 
					printf("change password successfully!\n");
					break;
				}
				
			}while(1);			
		}
	}while(1);
	
	return 0;
}