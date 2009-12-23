// Third-party platform access Authentication module.
// Copyright(C) 2009 LineKong.com
// @file pfauth.h
// @author: Benjamin
// @date: 2009-06-02

#ifndef _PLATFORM_AUTH_H
#define _PLATFORM_AUTH_H

#include "socket.h"
#include "logger.h"
#include "mutex.h"
#include "connector.h"

#ifndef MAX_PATH
#   define MAX_PATH 510
#endif


struct PfAuthCfg
{
    char    log_path[MAX_PATH];
    int     log_level;
    int     log_file_sz;

    int 	pool_size;
    int		queue_max;

    ConnProp    conn_prop;
};


class PfAuth
{
public:
    inline static PfAuth* singleton();
    int initialize(const char* conf_path);
    int release();
    Connector* createConn();
    void releaseConn(Connector* conn);
    uos::Logger& logger()
    {
        return _log;
    }

private:
    PfAuth(){};
    ~PfAuth(){};

    static PfAuth* _instance;
    static uos::Mutex _mtx;

    PfAuthCfg   _cfg;
    uos::Logger _log;

    friend class Connector;

};

inline PfAuth* PfAuth::singleton()
{
    // "Double-Checked Locking" idiom
    if (0 == _instance)
    {
        _mtx.lock();
        if (0 == _instance)
            _instance = new PfAuth;
        _mtx.unlock();
    }
    return _instance;
}

#endif//(_PLATFORM_AUTH_H)

