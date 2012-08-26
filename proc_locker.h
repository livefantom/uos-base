// The ProcessLocker class provides single instance control by file locking.
// Copyright(C) 2012 Tencent.com
// @file proc_locker.h
// @author: Benjamin
// @date: 2012-08-26

#ifndef _UOS_PROC_LOCKER_H
#define _UOS_PROC_LOCKER_H 

#include "uosdef.h"


_UOS_BEGIN

class ProcessLocker
{
public:
	explicit ProcessLocker(const char* path);
	~ProcessLocker() { unlock(); }

	void unlock();
	void lock();

private:
	DISABLE_COPY(ProcessLocker)
	int _pid;
	int _lock_fd;
	char _path[MAX_PATH+1];
};



_UOS_END


#endif//(_UOS_PROC_LOCKER_H)

