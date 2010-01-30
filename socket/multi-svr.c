#include <pthread.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <socket.h>

#define MAX_LINE	1024*4

#define _USE_FORK
//#define _USE_PTHREAD

char sndbuf[MAX_LINE] = "HTTP/1.1 200 OK\n"
"Date: Tue, 08 Jul 2008 13:04:53 GMT\n"
"Server: Apache/2.2.4 (Unix)\n"
"Content-Length: 1\n"
"Keep-Alive: timeout=5, max=100\n"
"Connection: Keep-Alive\n"
"Content-Type: text/html; charset=GBK\n\n";

// sighandler_t
void sig_chld(int signo)
{
	pid_t	pid;
	int	status;
	while( (pid = waitpid(-1, &status, WNOHANG)) > 0 )
		printf("pid:%ld> sig_chld> child:%ld terminated, status:%d\n", (long)getpid(), (long) pid, status);
}

void* str_echo(void* arg)
{
	ssize_t		rcvlen;
	char		rcvbuf[MAX_LINE];
	int* 		sockfd = (int*)arg;
#ifdef _USE_FORK
	pid_t		pid = getpid();
#elif defined(_USE_PTHREAD)
	pthread_t	pid = pthread_self();
#endif
	srand(pid);
	printf("\t\\_ pid:%ld> child starting...\n", (long) pid);
	while(1)
	{
		rcvlen = read(*sockfd, rcvbuf, MAX_LINE);
//		printf("\t\\_ pid:%ld> received %d bytes\n", (long) pid, rcvlen);
		if (rcvlen > 0)
		{
			int sec = rand()%20;
			printf("\t\\_ pid:%ld> sleep %d seconds...\n", (long) pid, sec);
			sleep( sec );	// wait for client receive timeout!!
			//write(*sockfd, rcvbuf, rcvlen);
			int offset = strlen(sndbuf);
			sprintf(sndbuf + offset, "%d", rand()%2);
			write(*sockfd, sndbuf, strlen(sndbuf));
			sndbuf[offset] = 0;
			//continue;
			break;
		}
		else if (rcvlen < 0 && errno == EINTR)
			continue;
		else if (rcvlen < 0)
			printf("\t\\_ pid:%ld> read error!\n", (long) pid);
		break;
	}
	close(*sockfd);
}

void* svr_func(void* arg)
{
	int* port = (int*)arg;
	struct sockaddr_in	svraddr, cltaddr;
	socklen_t		cltlen;
	int 			listenfd, connfd;
#ifdef _USE_FORK
	pid_t			childpid;
	pid_t			mypid = getpid();
#elif defined(_USE_PTHREAD)
	pthread_t		childpid;
#endif
	int			status;

    uos::Socket lisn_sock;
    lisn_sock.socket();
    lisn_sock.bind(SockAddr(0, *port));
    lisn_sock.listen();

	printf("pid:%ld> listen at port:%d, backlog:%d\n", (long)mypid, *port, SOMAXCONN);
#ifdef _USE_FORK
	signal(SIGCHLD, sig_chld);
#endif
	while(1)
	{
		cltlen = sizeof(cltaddr);
		// int accept(int sockfd, sturct sockaddr* cliaddr, socklen_t* addrlen);
		if ( -1 == (connfd = accept(listenfd, (struct sockaddr*) &cltaddr, &cltlen)) )
		{
			if (errno = EINTR)	// this may occur when sighandler return!
				continue;
			else
				printf("pid:%ld> accept error:%d\n", getpid(), errno);
		}
		// ### if no new connections, this accept() will block!!!
#ifdef _USE_FORK
		// pid_t fork(void)
		if ( 0 == (childpid = fork()) )
		{// child process
			// int close(int sockfd);
			close(listenfd);
			str_echo( (void*) &connfd );
			exit(0);
		}
		close(connfd);// passed it to child, so close it!!

#elif defined(_USE_PTHREAD)
		pthread_create(&childpid, NULL, str_echo, (void*) &connfd);
#endif
	}
}


int main(int argc, char** argv)
{
	pid_t	childpid;
	int	childnum = 10;
	int	port = 11001;
	int 	i = 0;
	int	status;
	for (; i < childnum; i++)
	{
#ifdef _USE_FORK
		if ( 0 == (childpid = fork()) )
		{
			svr_func(&port);
			exit(0);
		}
#endif
		port++;
	}
	while( (childpid = waitpid(-1, &status, WNOHANG)) > 0 )
		printf("pid:%ld> child terminated, status:%d\n", (long) childpid, status);
	return 0;
}

