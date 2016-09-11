#ifndef struct_h
#define struct_h

#include <stdio.h>

#define MAXLEN_CONTENT 1024
#define NAME_LEN 30
#define PASSWORD_LEN 11	
#define ACCOUNT_LEN 11	

#define LOGIN  1
#define ENROLL 2
#define CHANGE_PASSWORD 3

//used to store
typedef struct
{
	char content[MAXLEN_CONTENT];
	int len;  //消息长度
	int id;  //信息的标号
	char from[ACCOUNT_LEN];   //用户账号
	char from_name[NAME_LEN];  //用户名
} history_msg;

//used to chatting 
typedef struct 
{
	char content[MAXLEN_CONTENT];
	char name[NAME_LEN];  //用户名
	int id;  //信息的标号
} chat_msg;


//注册信息结构体
typedef struct
{
	char account[ACCOUNT_LEN];
	char password[PASSWORD_LEN];
	char name[NAME_LEN];	
} enroll; 

//登陆信息结构体
typedef struct 
{
	char account[ACCOUNT_LEN];
	char password[PASSWORD_LEN];
	int lmsg_id;
} login;

//修改密码结构体
typedef struct 
{
	char account[ACCOUNT_LEN];
	char password[PASSWORD_LEN];
} change_password;


//通信包
typedef struct 
{
	unsigned int  type;
	union
	{
		enroll enro;
		login log;
		change_password change_pws;
	} data;
} pck;

#endif
