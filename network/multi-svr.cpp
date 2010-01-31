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
		printf("pid:%ld> sig_chld | child:%ld terminated, status:%d\n", (long)getpid(), (long) pid, status);
}

void* str_echo(void* arg)
{
	char		rcvbuf[MAX_LINE];
    char        sndbuf[MAX_LINE];
    int		    rcvlen(MAX_LINE);
    int         sndlen;
    uos::Socket* sock_ptr = (uos::Socket*)arg;

    pid_t pid = getpid();
    printf("\t\\_ pid:%ld> child starting...\n", (long) pid);
    srand(pid);
	do
	{
        rcvlen = sock_ptr->recv(rcvbuf, rcvlen);
        if (rcvlen <= 0)
        {
            break;
        }
        printf("\t\\_ pid:%ld> received %d bytes\n", (long) pid, rcvlen);
		int sec = rand()%20;
		printf("\t\\_ pid:%ld> sleep %d seconds...\n", (long) pid, sec);
		sleep( sec );	// wait for client receive timeout!!

        //memcpy(sndbuf, rcvbuf, rcvlen);
		snprintf( sndbuf, MAX_LINE, "%s%d", http_resp, rand()%2 );

        sndlen = sock_ptr->send_all(sndbuf, strlen(sndbuf));
        if (sndlen > 0)
        {
            printf("\t\\_ pid:%ld> sent %d bytes\n", (long) pid, sndlen);
        }
		//continue;
	} while(false);
    sock_ptr->close();
    return 0;
}

void* svr_func(void* arg)
{
	int* port = (int*)arg;
	pid_t		childpid;
	int			status;

    signal(SIGCHLD, sig_chld);

    uos::Socket lisn_sock;
    lisn_sock.socket();
    if (lisn_sock.bind(uos::SockAddr((uint32_t)0, *port)) != 1)
    {
        return 0;
    }
    if (lisn_sock.listen() != 1)
    {
        return 0;
    }
    printf("pid:%ld> listen at port:%d, backlog:%d\n", getpid(), *port, SOMAXCONN);

    uos::Socket conn_sock;
    uos::SockAddr conn_addr((uint32_t)0, 0);
	while(1)
	{
        if (lisn_sock.accept(conn_sock, conn_addr) != 1)
        {
            continue;
        }
        // ### if no new connections, this accept() will block!!!

        // pid_t fork(void)
		if ( 0 == (childpid = fork()) )
		{// child process
            lisn_sock.close();
			str_echo( (void*) &conn_sock );
			exit(0);
		}
		conn_sock.close();// it was passed it to child by fork(), so close it!!

	}
}


int main(int argc, char** argv)
{
	pid_t	childpid;
	int	status;
    int	port = (argc > 1) ? atoi(argv[1]) : 11001;
    int	childnum = (argc > 2) ? atoi(argv[2]) : 10;

    for (int i(0); i < childnum; ++i)
	{
		if ( 0 == (childpid = fork()) )
		{
			svr_func(&port);
			exit(0);
		}
		port++;
	}
	while( (childpid = waitpid(-1, &status, WNOHANG)) > 0 )
		printf("pid:%ld> child terminated, status:%d\n", (long) childpid, status);
	return 0;
}

