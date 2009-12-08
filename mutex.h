// The Mutex class provides access serialization between threads.
// Copyright(C) 2009 LineKong.com
// @file mutex.h
// @author: Benjamin
// @date: 2009-07-02

#ifndef _UOS_MUTEX_H
#define _UOS_MUTEX_H 

#include "uosdef.h"

#ifdef WIN32
#else
#	include <pthread.h>
#endif



_UOS_BEGIN

class Mutex
{
public:
    explicit Mutex(bool recursive = false);
    ~Mutex();

    void lock();
    bool tryLock();
    void unlock();

private:
	DISABLE_COPY(Mutex)
#ifndef WIN32
    pthread_mutex_t _mutex;
#endif
};

class MutexLocker
{
public:
    inline explicit MutexLocker(Mutex* m) : _mtx(m) { relock(); }
    inline ~MutexLocker() { unlock(); }

    inline void unlock()
    {
        if (_mtx)
        	_mtx->unlock();
    }

    inline void relock()
    {
        if (_mtx)
            _mtx->lock();
    }

    inline Mutex* mutex() const
    {
        return _mtx;
    }

private:
    DISABLE_COPY(MutexLocker)
    Mutex* _mtx;
};



_UOS_END


#endif//(_UOS_MUTEX_H)


