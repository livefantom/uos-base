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

//////////////////////////////////////////////////////////////////////////
// class WaitCond members

WaitCond::WaitCond()
{
	init();


}

WaitCond::~WaitCond()
{

	cleanup();

}

void WaitCond::init()
{
}

void WaitCond::cleanup()
{
}

void WaitCond::lock()
{
}

void WaitCond::unlock()
{
}

void WaitCond::wakeOne()
{
	lock();

	unlock();
}

void WaitCond::wakeAll()
{
	lock();

	unlock();
}

bool WaitCond::wait(unsigned long millisecs)
{
	lock();
	int ret;
	unlock();
	

	return (ret == 0);
}

//////////////////////////////////////////////////////////////////////////
// class Thread members

void Thread::start()
{
	if (_running)
	{
		return;
	}

	_running  = true;
	_finished = false;
	_terminated = false;

	
}

bool Thread::wait(unsigned int time/* = ULONG_MAX */)
{
	if ( _thr_id == GetCurrentThread() )
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

bool Thread::running() const
{
	return _running;
}

bool Thread::finished() const
{
	return _finished;
}

unsigned int Thread::stackSize() const
{
    //MutexLocker locker(_mtx);
    return _stack_sz;
}


void Thread::sleep( unsigned long secs )
{
}

void Thread::msleep( unsigned long msecs )
{
}

void Thread::usleep( unsigned long usecs )
{
}

void Thread::terminate()
{
    if (!_thr_id)
	{
        return;
	}

}

void* Thread::innerStart(void* arg)
{
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

