#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define MAX_LINE 1024*4

struct ipaddr
{
	char ip[INET_ADDRSTRLEN];	// NET_ADDRSTRLEN = 16
	unsigned short port;
};

void* clt_func(void* arg)
{
	int	connfd;
	struct sockaddr_in	svraddr;
	struct ipaddr*	svrip = arg;
	char	sendbuf[MAX_LINE] = {0};
	int	sendtimes = 3;
	int	i = 0;
	// int socket(int domain, int type, int protocol); 
	connfd = socket(AF_INET, SOCK_STREAM, 0);
	svraddr.sin_family = AF_INET;
	// int inet_pton(int af, const char* src, void* dst);
	if ( inet_pton(AF_INET, svrip->ip, &svraddr.sin_addr) != 1 )
	{
		printf("ip convert error!\n");
		return 0;
	}
	// uint16_t htons(uint16_t hostshort);
	svraddr.sin_port = htons(svrip->port);
	// int connect(int sockfd, struct sockaddr* servaddr, socklen_t addrlen);
	if ( -1 == connect(connfd, (struct sockaddr*) &svraddr, sizeof(svraddr)) )
	{
		//printf("connect error:%d\n", errno);
		printf("pid:%ld> connect to %s:%d error:%d:%s\n",
			(long) getpid(), svrip->ip, svrip->port, errno, strerror(errno));
		return 0;
	}
	printf("pid:%ld> connected to %s:%d\n", (long) getpid(), svrip->ip, svrip->port);
	for (; i < sendtimes; i++)
	{
		struct timeval stv;
		struct timezone stz;
		uint64_t begin_time;
		uint64_t end_time;
		memset(sendbuf, 0, MAX_LINE);
		sprintf(sendbuf, "pid:%ld> %d\n", getpid(), i);
		// record send time.
		gettimeofday(&stv, &stz);
		begin_time = stv.tv_usec + (stv.tv_sec * 1000000);
		// ssize write(int fd, const void* buf, size_t count);
		if ( -1 == write(connfd, sendbuf, strlen(sendbuf)) )
		{
			printf("send data error:%d:%s\n", errno, strerror(errno));
			return 0;
		}
		memset(sendbuf, 0, MAX_LINE);
		// sszie read(int fd, void* buf, size_t count);
		if ( -1 == read(connfd, sendbuf, MAX_LINE) )
		{
			printf("receive data error:%d\n", errno);
			return 0;
		}
		else
		{
			// record send time.
			gettimeofday(&stv, &stz);
			end_time = stv.tv_usec + (stv.tv_sec * 1000000);
			printf("pid:%ld> clt | %llu <<<< %s\n", getpid(), end_time-begin_time, sendbuf);
		}
		sleep(1);
	}
}

int main(int argc, char** argv)
{
	printf("INET_ADDRSTRLEN = %d\n", INET_ADDRSTRLEN);
	struct ipaddr	svrip;
	pid_t	childpid;
	int	svr_num = 10;
	int	clt_num = 3;
	int	i, j;
	int	status;
	// read from cli arguments.
	if (argc > 1)
		sprintf(svrip.ip, "%s", argv[1]);
	else
		sprintf(svrip.ip, "192.168.1.229");
	if (argc > 2)
		svrip.port = atoi(argv[2]);
	else
		svrip.port = 11001;
	if (argc > 3)
		svr_num = atoi(argv[3]);
	if (argc > 4)
		clt_num = atoi(argv[4]);
	// create (10 * svrnum) client.
	j = 0;
	for (; j < svr_num; j++)
	{
		i = 0;
		for (; i < clt_num; i++)
		{
			// pid_t fork(void)
			if ( 0 == (childpid = fork()) )
			{// child process
				(*clt_func)(&svrip);
				exit(0);
			}
			// pid_t waitpid(pid_t pid, int* status, int options);
			childpid = waitpid(0, &status, WNOHANG);
			if (childpid > 0)
				printf("pid:%ld> child exit, status:%d\n", childpid, status);
		}
		svrip.port ++;
	}
	// wait for all child exit.
	while(1)
	{
		// pid_t wait(int* status);
		childpid = wait(&status);
		if (childpid > 0)
			printf("pid:%ld> child exit, status:%d\n", childpid, status);
		else
		{
			printf("wait error:%d", errno);
			break;
		}
	}
}
