#include "thread.h"
#include <cstdio>
#include <map>
#include "connector.h"
#include "pfauth.h"


int main()
{

	PfAuth* auth = PfAuth::singleton();
	auth->initialize("./pfauth.ini");
	Connector* conn = auth->createConn();
	int i = 0;
	while(1)
	{
		AuthMsg msg;
		msg.user_id = "31504325";
		msg.user_name = "zhy770129";
		msg.time = "1260247189";
		msg.flag = "be6fe47bdda836b8cebb27e869e67f7c";
		msg.state = 0;
		msg.retcode = -1;
		msg.adult = 0;
		conn->sendRequest(msg, ++i);
		printf("request sended: seq = %d\n", i);
		
		AuthMsg msg1;
		uint32_t seq = 0;
		int ret = conn->recvResponse(msg1, &seq);
		if (ret == 1)
		{
			printf("response recevied: seq = %d\n", seq);
		}

		usleep(100*1000);
	}// end of while.
	auth->releaseConn(conn);
	auth->release();

    return 0;
}
