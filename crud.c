#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mysql/mysql.h>
#include "crud.h"
#define ERR_EXIT(m)\
	do {perror(m);\
	exit(EXIT_FAILURE);\
	} while(0)
MYSQL mysql,*pconnection;
int sql_connect(const char *User, const char *Password, const char *DBName)
{
	mysql_init(&mysql);
	pconnection = mysql_real_connect(&mysql,"localhost",User,Password,DBName,0,0,0);
	if(pconnection == NULL)
	{
		ERR_EXIT(mysql_error(&mysql));
	}
	return 0;
}
int sql_disconnect()
{
	mysql_close(pconnection);
	return 0;
}
int query_result(const char *name, char **buf)
{
	int ret = mysql_query(pconnection,"set names utf8");
	if(ret != 0)
	{
		return -1;
	}
	char SQL[1024];
	memset(SQL, 0, sizeof(SQL));
	sprintf(SQL,"select url, name, description from baidu where name like '%%%s%%'",name);	
	printf("%s\n", SQL);
	ret = mysql_query(pconnection,SQL);
	if(ret != 0)
	{
		printf("mysql_query退出\n");
		return -1;
	}
	MYSQL_RES *result=NULL;
	result = mysql_store_result(pconnection);
	if(result == NULL)
	{
		return -1;
	}
	MYSQL_ROW row;
	row = mysql_fetch_row(result);
	if (row == NULL)
	{
		return -1;
	}
	*buf = malloc(8192);
	if(*buf == NULL)
		ERR_EXIT("malloc");
	memset(*buf,0,8192);
	char *tmp = *buf;
	while (1)
	{
		//printf("mysql_store_result不退出\n");
		char* c="<br>";
		row = mysql_fetch_row(result);
		if (row == NULL)
			break;
		char completeData[1024] = {0};	
		sprintf(completeData,HYPERLINK,row[0],row[1],row[2]);
		memcpy(&tmp[strlen(tmp)],completeData,strlen(completeData));
		memcpy(&tmp[strlen(tmp)],c,strlen(c));
	}
	tmp[strlen(tmp)] = '\0';
	mysql_free_result(result);
	//printf("%s\n",*buf);
	return 0;
}		
		/*sprintf(completeData,HYPERLINK,eachline);这是不行的
		int i;
		char eachline[1024]={0};
		char completeData[1024] = {0};
		char allline[1024]={0};
		for(i = 0; i < 3 ;i++)
		{
			char c = ',';
			memcpy(&eachline[strlen(eachline)],"aaA",sizeof("aaa"));
			memcpy(&eachline[strlen(eachline)],&c,sizeof(char));
		} 
		eachline[strlen(eachline)-1]='\0';
		printf("%s\n", eachline);
		memcpy(&completeData[strlen(completeData)],"this %s is %s good %s,",strlen("this %s is %s good %s,"));
		memcpy(&completeData[strlen(completeData)],eachline,strlen(eachline));
		completeData[strlen(completeData)]='\0';
		printf("%s\n", completeData);//打印出来是this %s is %s good %s,aaA,aaA,aaA
		*/

