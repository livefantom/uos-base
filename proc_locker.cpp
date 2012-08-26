// The Mutex class provides access serialization between threads.
// Copyright(C) 2009 LineKong.com
// @file mutex.h
// @author: Benjamin
// @date: 2009-07-02

#include "proc_locker.h"
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define	Warning	printf


_UOS_BEGIN


ProcessLocker::ProcessLocker(const char* path)
 : _pid(0)
 , _lock_fd(-1)
{
	if (NULL == path || strlen(path) < 1 || strlen(path) > MAX_PATH)
	{
		Warning("ProcessLocker::ProcessLocker: bad file path: %s", path);
	}

	strncpy(_path, path, MAX_PATH);
	_path[strlen(_path)] = 0;

	lock();
}

void ProcessLocker::lock()
{
	// open file.
	_lock_fd = open(_path, O_RDWR | O_CREAT, 0644);
	if (-1 == _lock_fd)
	{
		Warning("ProcessLocker::ProcessLocker: Open lock file \"%s\" FAILED\nApp exit. \n", _path);
		exit(1);
	}

	char buff[32] = {0};
	int nbytes = 0;
	// lock file.
	if (-1 == flock(_lock_fd, LOCK_EX | LOCK_NB) )
	{
		nbytes = read(_lock_fd, buff, sizeof(buff)-1);
		buff[nbytes] = 0;
		Warning("ProcessLocker::lock: Process %s already running in current working directory, \nThis instance exit. \n", buff);
		exit(1);
	}
	if (-1 == ftruncate(_lock_fd, 0))
	{
		Warning("ProcessLocker::lock: Clear lock file \"%s\" FAILED\nApp exit. \n", _path);
		exit(1);
	}

	// write pid to file.
	_pid = getpid();
	snprintf(buff, sizeof(buff)-1, "%d", _pid);
	nbytes = strlen(buff);
	buff[nbytes] = 0;
	if (write(_lock_fd, buff, nbytes) != nbytes)
	{
		Warning("ProcessLocker::lock: Write PID(%d) to lock file \"%s\" FAILED\nApp exit. \n", _pid, _path);
		exit(1);
	}

	Warning("ProcessLocker::lock: Instance (PID = %d) lock OK\n", _pid);
}

void ProcessLocker::unlock()
{
	do
	{
		if (-1 == _lock_fd)
		{
			Warning("ProcessLocker::unlock: bad file description.\n");
			break;
		}

		// unlock file.
		if (-1 == flock(_lock_fd, LOCK_UN | LOCK_NB) )
		{
			Warning("ProcessLocker::unlock: flock() failure: %s", strerror(errno));
			break;
		}

		// close file.
		if (-1 == close(_lock_fd))
		{
			Warning("ProcessLocker::unlock: file `%s' close() failure: %s", _path, strerror(errno));
			break;
		}

		// remove file.
		if (-1 == remove(_path))
		{
			Warning("ProcessLocker::unlock: file `%s' remove() failure: %s", _path, strerror(errno));
			break;
		}

		printf("Instance (PID = %d) unlock OK\n", _pid);
	} while(false);
}



_UOS_END


