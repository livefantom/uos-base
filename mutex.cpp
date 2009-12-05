// The Mutex class provides access serialization between threads.
// Copyright(C) 2009 LineKong.com
// @file mutex.h
// @author: Benjamin
// @date: 2009-07-02

#include "mutex.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define	Warning	printf


_UOS_BEGIN


Mutex::Mutex(bool recursive /* = false */)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);

	if (recursive)
	{
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	}
	else
	{
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_FAST_NP);
	}

	int ret = pthread_mutex_init(&_mutex, &attr);

	pthread_mutexattr_destroy(&attr);

	if(ret) Warning("Mutex::Mutex: init failure: %s", strerror(ret));

}

Mutex::~Mutex()
{
	int ret = pthread_mutex_destroy(&_mutex);

	if (ret) Warning("Mutex::~Mutex: destroy failure: %s", strerror(ret));

}

void Mutex::lock()
{
	int ret = pthread_mutex_lock(&_mutex);

	if (ret) Warning("Mutex::lock: mutex lock failure: %s", strerror(ret));
}

void Mutex::unlock()
{
	int ret = pthread_mutex_unlock(&_mutex);

	if (ret) Warning("Mutex::unlock: mutex unlock failure: %s", strerror(ret));
}

bool Mutex::tryLock()
{
	int ret = pthread_mutex_trylock(&_mutex);

	if (0 == ret)
   	{
		return true;
	}
	else if (ret != EBUSY)
   	{
		Warning("Mutex::locked: try lock failed: %s", strerror(ret));
	}

	return false;
}




_UOS_END


