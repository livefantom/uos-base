// Light weight thread class interface.
// Copyright(C) 2009 LineKong.com
// @file thread.h
// @author: Benjamin
// @date: 2009-05-02

#ifdef WIN32
#else
#	include <unistd.h>
#	include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define	Warning	printf

#include "thread.h"

_UOS_BEGIN

//////////////////////////////////////////////////////////////////////////
// class WaitCond members

WaitCond::WaitCond()
{
	init();

	pthread_condattr_t cattr;
	pthread_condattr_init( &cattr );

	int ret = pthread_cond_init( &_cond, &cattr );

	pthread_condattr_destroy( &cattr );

	if (ret) Warning("WaitCond::WaitCond: event init failure %s", strerror( ret ));
}

WaitCond::~WaitCond()
{
	int ret = pthread_cond_destroy( &_cond );

	if (ret)
	{
		pthread_cond_broadcast( &_cond );
	}

	cleanup();

}

void WaitCond::init()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_DEFAULT );

	int ret = pthread_mutex_init( &_mutex, &attr );

	pthread_mutexattr_destroy( & attr );

	if (ret) Warning("WaitCond::init: mutex init failure: %s", strerror(ret));
}

void WaitCond::cleanup()
{
	int ret = pthread_mutex_destroy( &_mutex );

	if (ret) Warning("WaitCond::cleanup: mutex destroy failure: %s", strerror(ret));
}

void WaitCond::lock()
{
	int ret = pthread_mutex_lock( &_mutex );

	if (ret) Warning("WaitCond::lock: mutex lock failure: %s", strerror(ret));
}

void WaitCond::unlock()
{
	int ret = pthread_mutex_unlock( &_mutex );

	if (ret) Warning("WaitCond::unlock: mutex unlock failure: %s", strerror(ret));
}

void WaitCond::wakeOne()
{
	lock();
	int ret = pthread_cond_signal( &_cond );

	if (ret) Warning("WaitCond::wakeOne: wake error: %s",strerror(ret));

	unlock();
}

void WaitCond::wakeAll()
{
	lock();
	int ret = pthread_cond_broadcast( &_cond );
	
	if (ret) Warning("WaitCond::wakeAll: wake error: %s",strerror(ret));

	unlock();
}

bool WaitCond::wait(unsigned long millisecs)
{
	lock();
	int ret;
	if (millisecs != ULONG_MAX)
	{
		struct timeval tv;
		gettimeofday(&tv, 0);
		
		timespec ti;
		ti.tv_sec = tv.tv_sec + (millisecs / 1000);
		ti.tv_nsec = (tv.tv_usec * 1000) + (millisecs % 1000) * 1000000;
		
		ret = pthread_cond_timedwait( &_cond, &_mutex, &ti );
	}
	else
	{
		ret = pthread_cond_wait( &_cond, &_mutex );
	}
	unlock();
	
	if ( ret && ret != ETIMEDOUT )
	{
		Warning("WaitCond::wait: wait error:%s",strerror(ret));
	}

	return (ret == 0);
}

//////////////////////////////////////////////////////////////////////////
// class Thread members

pthread_t Thread::currentThreadId()
{
    return pthread_self();
}

void Thread::start()
{
    MutexLocker locker(&_mtx);
	if (_running)
	{
		return;
	}

	_running  = true;
	_finished = false;
	_terminated = false;

#ifndef NO_DEBUG
	Warning("create thread\n");
#endif
	pthread_attr_t  attr;
	
	pthread_attr_init( &attr );
	pthread_attr_setinheritsched( &attr, PTHREAD_INHERIT_SCHED );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

	if ( _stack_sz > 0 )
	{
		int ret = pthread_attr_setstacksize( &attr, _stack_sz );
		if (ret)
		{
			Warning("Thread::start: Thread stack size error: %s", strerror(ret));
            _running = false;
            _finished = false;
            return;
		}
	}
	
	int ret = pthread_create( &_thr_id, &attr, Thread::innerStart, this);
	
	pthread_attr_destroy( &attr );
	
	if (ret)
	{
		Warning("Thread::start: Thread creation error: %s", strerror(ret));
		_running  = false;
		_finished = false;
		_thr_id = 0;
	}
}

bool Thread::wait(unsigned int time/* = ULONG_MAX */)
{
	if ( _thr_id == pthread_self() )
	{
		Warning("Thread::wait: Thread tried to wait on itself");
		return false;
	}

	if ( _finished || !_running )
	{
		return true;
	}

	return _thr_done.wait(time);
}

bool Thread::isRunning() const
{
    MutexLocker locker(&_mtx);
	return _running;
}

bool Thread::isFinished() const
{
    MutexLocker locker(&_mtx);
	return _finished;
}

void Thread::setStackSize( unsigned int stackSize )
{
    MutexLocker locker(&_mtx);
	_stack_sz = stackSize;
}

unsigned int Thread::stackSize() const
{
    MutexLocker locker(&_mtx);
    return _stack_sz;
}

// internal helper function to do thread sleeps, since usleep()/nanosleep()
// aren't reliable enough (in terms of behavior and availability)
static void thread_sleep(struct timespec *ti)
{
    pthread_mutex_t mtx;
    pthread_cond_t cnd;

    pthread_mutex_init(&mtx, 0);
    pthread_cond_init(&cnd, 0);

    pthread_mutex_lock(&mtx);
    (void) pthread_cond_timedwait(&cnd, &mtx, ti);
    pthread_mutex_unlock(&mtx);

    pthread_cond_destroy(&cnd);
    pthread_mutex_destroy(&mtx);
}

void Thread::sleep( unsigned long secs )
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    struct timespec ti;
    ti.tv_sec = tv.tv_sec + secs;
    ti.tv_nsec = (tv.tv_usec * 1000);
    thread_sleep(&ti);
}

void Thread::msleep( unsigned long msecs )
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    struct timespec ti;

    ti.tv_nsec = (tv.tv_usec + (msecs % 1000) * 1000) * 1000;
    ti.tv_sec = tv.tv_sec + (msecs / 1000) + (ti.tv_nsec / 1000000000);
    ti.tv_nsec %= 1000000000;
    thread_sleep(&ti);
}

void Thread::usleep( unsigned long usecs )
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    struct timespec ti;

    ti.tv_nsec = (tv.tv_usec + (usecs % 1000000)) * 1000;
    ti.tv_sec = tv.tv_sec + (usecs / 1000000) + (ti.tv_nsec / 1000000000);
    ti.tv_nsec %= 1000000000;
    thread_sleep(&ti);
}

void Thread::terminate()
{
    if (!_thr_id)
	{
        return;
	}

    int ret = pthread_cancel(_thr_id);
    if (ret)
   	{
        Warning("Thread::terminate: Thread termination error: %s", strerror(ret));
    }
   	else
   	{
        _terminated = true;
    }
}

void* Thread::innerStart(void* arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_cleanup_push(Thread::finish, arg);

	Thread *thr = reinterpret_cast<Thread*>(arg);

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    thr->run();

    pthread_cleanup_pop(1);
    return 0;
}

void Thread::finish(void* arg)
{
	Thread *thr = reinterpret_cast<Thread*>(arg);
#ifndef NO_DEBUG
	Warning("thread finish\n");
#endif

	thr->_running  = false;
	thr->_finished = true;
    thr->_terminated = false;

    thr->_thr_id = 0;
	thr->_thr_done.wakeAll();
}

_UOS_END




