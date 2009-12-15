#include "thread.h"
#include <cstdio>
#include <map>
#include "connector.h"
#include "pfauth.h"


int main()
{

	PfAuth* auth = PfAuth::singleton();
	auth->initialize("pfauth.conf");
	Connector conn = auth->createConn();
	while(1)
	{
		++i;
		AuthMsg msg;
		msg.val = i;
		msg.state = 0;

		conn->setRequest();
		conn->getResponse();

		usleep(100*1000);
	}// end of while.
	auth->release();

    return 0;
}
