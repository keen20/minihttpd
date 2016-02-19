#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "work.h"
#include "pub.h"
#include "crud.h"

//8192是8k
#define BUFSIZE 8192
//HTTP 回复
#define HEAD "HTTP/1.0 200 OK\n\
Content-Type: %s\n\
Transfer-Encoding: chunked\n\
Connection: Keep-Alive\n\
Accept-Ranges:bytes\n\
Content-Length:%d\n\n"
//消息尾两个回车
#define TAIL "\n\n"
//post请求在URL中表示
#define EXEC "s?wd="

void gethttpcommand(const char *sHTTPMsg, char *command) //从http请求中读出GET后面的命令行
{
	int i;
	int istart = 0;
	int iend = 0;
	for (i = 0; i < strlen(sHTTPMsg); i++)
	{//GET /abc.txt HTTP/1.1（/abc.txt两边有两个空格）
		if ((sHTTPMsg[i] == ' ') && (istart == 0))//找到GET后第一个空格
		{
			istart = i + 2;//跳过'/'直接指向资源abc.txt
		} else
		{
			if (sHTTPMsg[i] == ' ')//找到资源abc.txt后的第一个空格
			{
				iend = i;
				break;
			}
		}
	}
	strncpy(command, &sHTTPMsg[istart], (iend - istart));//获取资源名
}

int gettempletcontent(char *buf) //得到模板文件templet.html的内容
{
	struct stat t;
	memset(&t, 0, sizeof(t));
	FILE *fd = fopen("templet.html", "rb");
	if (fd != NULL)
	{
		stat("templet.html", &t);
		fread(buf, t.st_size, 1, fd);
		return t.st_size;
	} else
	{
		printf("open %s failed %s\n", "templet.html", strerror(errno));
		return 0;
	}
}

int getdynamicccontent(const char *query, char **buf) //动态设置http请求内容,query为条件，buf为动态内容(存入buf)
{//如果是一级指针buf是char*类型，里面存放的是char类型，而malloc要返回指针
	char templetcontent[1024];
	memset(templetcontent, 0, sizeof(templetcontent));
	//获取模板
	if (gettempletcontent(templetcontent) == 0)//没有得到模板内容
		return 0;

	*buf = malloc(BUFSIZE);
	char *body = NULL;

	if (query_result(query, &body) == -1)
	{
		body = malloc(128);
		memset(body, 0, 128);
		strcpy(body, "抱歉，没有查询结果");
		printf("查询吴国body:%s\n",body);
	}
	//数据填充入模板页
	sprintf(*buf, templetcontent/*里面有%S*/, query, body);
	printf("查询吴国buf:%s\n",*buf);
	free(body);
	return strlen(*buf);
}

//根据URL请求，生成静态http reponse消息内容
int make_http_content(const char *command/*URL后面命令*/, char **buf) 
{
	char *contentbuf = NULL;
	int icontentlen = 0;

	if (command[0] == 0) //GET请求后面为空，得到默认页面内容图
	{
		icontentlen = getfilecontent("default.html", &contentbuf);
	} else
	{
		if (strncmp(command, EXEC, strlen(EXEC)) == 0) //如果请求后面为s?wd=，则获取其后的资源值
		{//收到的请求是GET s?wd=%AD%AB%AD%AB%AD%AB%AD%AB HTTP/1.1
			char query[1024];
			memset(query, 0, sizeof(query));
			httpstr2stdstr(&command[strlen(EXEC)], query); //得到s?wd=字符串后面的转义字符内容
			if(command[strlen(EXEC)] == 0)
			{
				icontentlen = getfilecontent("default.html", &contentbuf);
			}
			else
			{
				icontentlen = getdynamicccontent(query, &contentbuf);
			}
		} else
		{
			icontentlen = getfilecontent("default.html", &contentbuf);
		}
	}

	if (icontentlen > 0)
	{
		char headbuf[1024];
		memset(headbuf, 0, sizeof(headbuf));
		sprintf(headbuf, HEAD, getfiletype(command), icontentlen); //设置消息头
		int iheadlen = strlen(headbuf);//得到消息头长度
		int itaillen = strlen(TAIL);//得到消息尾长度
		int isumlen = iheadlen + icontentlen + itaillen;//得到消息总长度
		*buf = malloc(isumlen);//根据消息总长度，动态分配内存
		char *tmp = *buf;
		memcpy(tmp, headbuf, iheadlen); //安装消息头
		memcpy(&tmp[iheadlen], contentbuf, icontentlen); //安装消息体
		memcpy(&tmp[iheadlen + icontentlen], TAIL, itaillen); //安装消息尾
		printf("回复头:\nheadbuf:\n%s", headbuf);
		/*printf("回复消息体:\ncontentbuf:\n%s", contentbuf);
		printf("回复消息尾:\nTAIL:\n%s", TAIL);*///每次都输出一样
		if (contentbuf)
			free(contentbuf);
		return isumlen;//返回消息总长度
	} else
	{
		return 0;
	}
}

void *socket_contr(void *arg)//线程入口函数
{
	printf("一个线程开始\n");
	int st = *(int *) arg;//得到来自client端的socket
	free((int *) arg);

	char buf[BUFSIZE];
	memset(buf, 0, sizeof(buf));
	int rc = recv(st, buf, sizeof(buf), 0);//接收来自client端socket的消息
	if (rc <= 0)
	{
		printf("recv failed %s\n", strerror(errno));
	} else
	{
		printf("接收到请求:\n%s", buf);
		char command[1024];
		memset(command, 0, sizeof(command));
		gethttpcommand(buf, command); //得到http 请求中 GET后面的字符串

		char *content = NULL;
		int ilen = make_http_content(command, &content);//根据用户在GET中的请求，生成相应的回复内容
		if (ilen > 0)
		{
			send(st, content, ilen, 0);//将回复的内容发送给client端socket
			free(content);
		}
	}
	close(st);//关闭client端socket
	printf("一个线程结束！\n");
	return NULL;
}

