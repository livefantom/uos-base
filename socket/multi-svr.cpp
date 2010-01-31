#include <pthread.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <socket.h>

#define MAX_LINE	1024*4


char http_resp[MAX_LINE] = "HTTP/1.1 200 OK\n"
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
    char        sndbuf[MAX_LINE];
	int* 		sockfd = (int*)arg;
	pid_t		pid = getpid();

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
#if 0
			memcpy(sndbuf, rcvbuf, rcvlen);
#else
			snprintf( sndbuf, MAX_LINE, "%s%d", http_resp, rand()%2 );
#endif
            write(*sockfd, sndbuf, strlen(sndbuf));
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
	pid_t			childpid;
	int			status;

    uos::Socket lisn_sock;

#ifdef _USE_FORK
    signal(SIGCHLD, sig_chld);
#endif

    lisn_sock.socket();
    lisn_sock.bind(SockAddr(0, *port));
    if (lisn_sock.listen() != 1)
        return -1;

    printf("pid:%ld> listen at port:%d, backlog:%d\n", getpid(), *port, SOMAXCONN);
    uos::Socket conn_sock;
    uos::SockAddr conn_addr;
	while(1)
	{
        if ( EINTR == lisn_sock.accept(conn_sock, conn_addr) ) // ### if no new connections, this accept() will block!!!
			continue;

        // pid_t fork(void)
		if ( 0 == (childpid = fork()) )
		{// child process
			// int close(int sockfd);
			close(listenfd);
			str_echo( (void*) &connfd );
			exit(0);
		}
		close(connfd);// it was passed it to child by fork(), so close it!!

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

