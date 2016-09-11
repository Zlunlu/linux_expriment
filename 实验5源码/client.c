#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <error.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include "struct.h"
#include "create_socket.h"

//int clientCreateSsocket();
int chatting(int sockfd);
int clientLogin(int sockfd);
int readData(char *account);
int mygets(char *str,int t);
void saveData();

#define PORT 4567

char dir[50]="./clientData/";
int *last_recv_msg_id;

//先看服务器的代码
int main(int argc,char **argv)
{
    int sockfd;
    
    sockfd=clientCreateSsocket();

    signal(SIGINT,saveData);
    clientLogin(sockfd);
    printf("waiting connect...\n");

    sleep(3);
    system("clear");
    printf("\n\n********Welcome to chat room!*********\n");
    chatting(sockfd);

    close(sockfd);
    return 0;
}

int readData(char *account)
{
    int fd;

    strcat(dir,account);
    fd=open(dir,O_CREAT|O_RDWR,00777);
    lseek(fd,sizeof(int)-1,SEEK_SET);
    write(fd,"",1);
    last_recv_msg_id=(int *)mmap(NULL,sizeof(int),PROT_READ|PROT_WRITE,
        MAP_SHARED,fd,0);
    printf("%d\n",*last_recv_msg_id);
    close(fd);    

    return 0;
}

void saveData()
{
    int fd;
    fd=open(dir,O_WRONLY);
    write(fd,last_recv_msg_id,sizeof(int));
    close(fd);
    exit(0);
}
/*
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
*/

int chatting(int sockfd)
{
    int pid;
    char buf[MAXLEN_CONTENT];
    int content_bytes;
    chat_msg msg;

    pid=fork();
    if(pid==-1)
    {
        perror("fork error!\n");
        close(sockfd);
        exit(1);
    }
    if(pid==0)
    {
        signal(SIGINT,SIG_DFL);
        while(1)
        {
            fgets(buf,MAXLEN_CONTENT,stdin);
            if(strcmp(buf,"\n")==0) //消除回车的影响 
                continue;
            //printf("send: %d",strlen(buf));
            if(send(sockfd,buf,strlen(buf),0)==-1)
            {
                perror("send error!\n");
                close(sockfd);
                exit(1);
            }
        }
    }
    if(pid>0)
    {   
        signal(SIGINT,SIG_DFL);       
        while(1)
        {
            memset(&msg,0,sizeof(chat_msg));  //消除干扰
            if((content_bytes=recv(sockfd,&msg,sizeof(chat_msg),0))==-1)
            {
                    perror("revc error!\n");
                    close(sockfd);
                    exit(1);
            }
            if(strlen(msg.content)>0 )
            {
                //printf("recv message!  %d\n",strlen(msg.content));
                //fputs(buf,stdout);
                printf("%s: ",msg.name);
                *last_recv_msg_id=msg.id;
                fputs(msg.content,stdout);
            }
        }
    }
    return 0;       
}

int clientLogin(int sockfd)
{
    int choose;

    //临时变量
    char password[PASSWORD_LEN];
    char name[NAME_LEN];
    char account[ACCOUNT_LEN];

    pck umsg;  //user message
    umsg.type = 0;
    
    enum flag { logined=1,unlogin=-1,enrolled=2,unenroll=-2,
        change_pws=3,unchange_pws=-3};
    do
    {
        
        printf("*********login************\n");
        printf("   1.login    2.enroll  3.change password  4.quit\n");
        scanf("%d",&choose);
        switch(choose)
        {
            case 2 : memset(&umsg,0,sizeof(pck));
                     umsg.type = ENROLL;
                     printf("account(不能输入空格): ");  
                     scanf("%s",account);
                     strcpy(umsg.data.enro.account,account);
                     printf("password(不能输入空格): ");
                     scanf("%s*c",password);
                     strcpy(umsg.data.enro.password,password);
                     printf("name(不能输入空格): ");
                     //fflush(stdin);
                     scanf("%s",name);
                     //fgets会受输入流中回车的干扰，无法输入
                     //fgets(name,sizeof(name),stdin);  
                     strcpy(umsg.data.enro.name,name);
                     send(sockfd,&umsg,sizeof(pck),0);
                     recv(sockfd,&umsg,sizeof(pck),0);
                     break;
            case 1:  memset(&umsg,0,sizeof(pck));
                     umsg.type = LOGIN;
                     printf("account(不能输入空格): ");
                     scanf("%s",account);
                     strcpy(umsg.data.log.account,account);
                     printf("password(不能输入空格): ");
                     scanf("%s",password);
                     strcpy(umsg.data.log.password,password);
                     send(sockfd,&umsg,sizeof(pck),0);
                     recv(sockfd,&umsg,sizeof(pck),0);
                     sleep(1);
                     break;
            case 3:  memset(&umsg,0,sizeof(pck));
                     umsg.type = CHANGE_PASSWORD;
                     printf("account(不能输入空格): ");
                     scanf("%s",account);
                     strcpy(umsg.data.log.account,account);
                     printf("password(不能输入空格): ");
                     scanf("%s",password);
                     strcpy(umsg.data.log.password,password);
                     send(sockfd,&umsg,sizeof(pck),0);
                     recv(sockfd,&umsg,sizeof(pck),0);
                     break;
            case 4: printf("Thank you for your using\n"); 
                    exit(0);

            default: printf("inputs wrong!Please again.\n"); 
                     break;                    

        }
        
        if(umsg.type==logined)  //LOGIN==logined
        {
            sleep(1);
            printf("login successfully!\n");
            readData(umsg.data.log.account);
            umsg.data.log.lmsg_id=*last_recv_msg_id;
            send(sockfd,&umsg,sizeof(umsg),0);
            break;
        }
        if(umsg.type==unlogin)
        {
            sleep(1);
            printf("account or password wrong,please login again!\n");
        }
        if(umsg.type == enrolled)
        {
            sleep(1);
            printf("enroll successfully! You can login now.\n");           
        }
        if(umsg.type == unenroll)
        {
            sleep(1);
            printf("the account have enrolled!\n");
        }
        if(umsg.type == change_pws)
        {
            printf("You can change your password now,please input:\n");
            printf("password: ");
            scanf("%s",password);
            strcpy(umsg.data.change_pws.password,password);
            send(sockfd,&umsg,sizeof(pck),0);
            //recv(sockfd,&umsg,sizeof(pck),0);  //need to think more
            sleep(1);
            printf("Your password have changed!\n");
        }
        if(umsg.type == unchange_pws)
        {
            sleep(1);
            printf("You input account or password error!\n");            
        }

    }while(1);
   
}

int mygets(char *str,int t)
{
    int i=0;
    while(t>1 && i<t)
    {
        scanf("%c",&str[i]);
        if(str[i]=='\n')
            break;
        i++;
        t--;
    }
    str[i]='\0';
    return 0;
}