#include "thread.h"
#include <cstdio>

class WorkThread : public uos::Thread
{
protected:
	virtual void run()
	{
		int i=10;
		while(i--)
		{
			printf("%d, %d\n", i, (int)time(0));
			sleep(1);
		}
	}
};

int main()
{
	WorkThread* pth = new WorkThread();
	pth->start();
	pth->wait();
	delete pth;
    return 0;
}
