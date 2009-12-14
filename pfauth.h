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
#include <map>

#ifndef MAX_PATH
#   define MAX_PATH 510
#endif


struct AuthMsg
{
	/*
	std::string time;
	std::string flag;
	std::string user_name;
	std::string user_id;
	*/
	int val;
	int state;
};


typedef std::map<int, AuthMsg> MsgMap;
typedef std::pair<int, AuthMsg> MsgPair;
typedef MsgMap::iterator MsgIter;


struct PfAuthCfg
{
    char    log_path[MAX_PATH];
    int     log_level;
    int     log_file_sz;

    ConnProperty    conn_proper;
};


class PfAuth
{
public:
    inline static PfAuth* singleton();
    int initialize(const char* conf_path);
    int release();
    Connector* createConn();
    void releaseConn(Connector* conn);
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

