#include "thread.h"
#include "connector.h"
#include "pfauth.h"
#include <SysTimeValue.h>
#include <signal.h>
#include <cstdio>


int g_stop = false;
int g_send = false;

void sigExit(int sig)
{
	printf("^C pressed, sig=%d!\n", sig);
	g_stop = true;
}

void sigSend(int sig)
{
	g_send = !g_send;
	printf("sig=%d received, g_send=%d!\n", sig, g_send);
}

int main()
{

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  sigExit);
    signal(SIGTERM, sigExit);
    signal(SIGUSR1, sigSend);

	PfAuth* auth = PfAuth::singleton();
	auth->initialize("./pfauth.ini");
	Connector* conn = auth->createConn();
	int i = 0;
	uint64_t last_send = 0;
	uint64_t now = 0;
	int ret;

	while( !g_stop )
	{
		AuthMsg msg;
		msg.user_id = "31504325";
		msg.user_name = "zhy770129";
		msg.time = "1260247189";
		msg.flag = "be6fe47bdda836b8cebb27e869e67f7c";
		msg.game_id = 707;
		msg.cmd_id = 0x10003801;

		SysTimeValue::getTickCount(&now);
		if ( now - last_send > 400 && g_send )
		{
			SysTimeValue::getTickCount(&last_send);
			ret = conn->sendRequest(msg, i);
			if (ret == 1)
			{
				printf("request sended: seq = %d\n", i);
				++i;
			}
		}

		AuthMsg msg1;
		uint32_t seq = 0;
		ret = conn->recvResponse(msg1, &seq);
		if (ret == 1)
		{
			printf("response recevied: seq=%d, retcode=%d, state=%d\n", seq, msg1.retcode, msg1.state);
		}
		usleep(10);

	}// end of while.
	auth->releaseConn(conn);
	auth->release();

    return 0;
}
