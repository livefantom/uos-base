#include "thread.h"
#include <cstdio>
#include <map>

MsgMap g_msg_map;

void getOneRequest(char* buffer, int* nbytes, int* sequence)
{
    MsgIter iter = g_msg_map.begin();
    while (!g_msg_map.empty() && iter != g_msg_map.end())
    {
        AuthMsg msg = iter->second;
        if (msg.state == 0)
        {
        	strcpy(buffer, "GET / HTTP1.1");
        	*nbytes = strlen(buffer);
        	*sequence = iter->first;
        	msg.state = 1;
            ++iter;
        }
    }

}

void setOneResponse(int retcode, int sequence)
{
	MsgIter iter = g_msg_map.find(sequence);
    if ( iter != g_msg_map.end() )
    {
        AuthMsg msg = iter->second;
        msg.val = retcode;
        msg.state = 2;
	}

}

class WorkThread : public uos::Thread
{
protected:
	virtual void run()
	{
/*
		int i=10;
		while(i--)
		{
			printf("%d, %d\n", i, (int)time(0));
			sleep(1);
		}
*/

		int i = 0;
		while(1)
		{
			++i;
			if ( g_msg_map.size() > 1024 )
			{
	        	printf("sent msg reponse to GS[no memory], sequence :%d", i);
			}
			else
			{
				AuthMsg msg;
				msg.val = i;
				msg.state = 0;
				g_msg_map.insert( MsgPair(i, msg) );
			}

			// check work.
		    MsgIter iter = g_msg_map.begin();
		    while (!g_msg_map.empty() && iter != g_msg_map.end())
		    {
		        //uint64_t ullTickCount = GetTickCount();
		        AuthMsg msg = iter->second;
		        //if (ullTickCount > msg.nLastTickCount + msg.nTimeout)
		        if (msg.state == 2)
		        {
		        	printf("sent msg reponse to GS[OK], sequence :%d", iter->first);
		            g_msg_map.erase(iter++);
		        }
		        else
		        {
		            ++iter;
		        }
		    }
			usleep(100*1000);
		}// end of while.
	}
};

int main()
{
	WorkThread* pth = new WorkThread();
	pth->start();

	PfAuth* auth = PfAuth::singleton();
	auth->initialize("pfauth.conf");
	auth->setRequest();
	auth->getResponse();

	pth->wait();
	delete pth;
    return 0;
}
