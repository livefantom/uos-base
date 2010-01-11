// Light weight thread class interface.
// Copyright(C) 2009 LineKong.com
// @file thread.h
// @author: Benjamin
// @date: 2009-05-02

#ifndef _UOS_THREAD_H
#define _UOS_THREAD_H 

#include "uosdef.h"
#include "mutex.h"

#ifdef WIN32
#ifndef ULONG_MAX
#	define ULONG_MAX 0xFFFFFFFF
#endif
#else
#	include <pthread.h>
#endif


_UOS_BEGIN


class WaitCond
{
public:
        WaitCond();
        ~WaitCond();
        void wakeOne();
        void wakeAll();
        bool wait(unsigned long millisecs);

private:
        void lock();
        void unlock();
        void init();
        void cleanup();

private:
#ifndef WIN32
        pthread_cond_t  _cond;
        pthread_mutex_t _mutex;
#endif
};

class Thread
{
public:
	static pthread_t currentThreadId();

	explicit Thread():_thr_id(0),_running(false),_finished(false),_stack_sz(0){}
	virtual ~Thread(){ _thr_id = 0; }

	void start();

	bool wait(unsigned int time = ULONG_MAX);

	void terminate();

	bool isRunning() const;
	bool isFinished() const;

    void setStackSize( unsigned int stackSize );
	unsigned int stackSize() const;

protected:
	virtual void run() = 0;

	static void sleep( unsigned long );
	static void msleep( unsigned long );
	static void usleep( unsigned long );

private:
#ifdef WIN32
    HANDLE      _thr_id;
#else
	pthread_t	_thr_id;
#endif
	WaitCond	_thr_done;

	mutable Mutex _mtx;

	bool	_running;
	bool	_finished;
	bool	_terminated;

	unsigned int	_stack_sz;

	static void* innerStart(void* arg);
	static void finish(void* arg);
};



_UOS_END


#endif//(_UOS_THREAD_H)




