// Third-party platform access Authentication module.
// Copyright(C) 2009 LineKong.com
// @file pfauth.cpp
// @author: Benjamin
// @date: 2009-06-02

#include "pfauth.h"
#include <OperationCode.h>
#include <SysConfig.h>
#include <signal.h>
#include "mutex.h"
#include "logger.h"

#ifdef E_ERROR
#   undef E_ERROR
#   define E_ERROR      -1
#endif

PfAuth* PfAuth::_instance = 0;
uos::Mutex PfAuth::_mtx;

int PfAuth::initialize(const char* conf_path)
{
    int retcode = E_ERROR;

    int line_count = 0;
    SysConfig conf;
#ifndef WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    retcode = conf.initialize();
    retcode = conf.load(conf_path, &line_count);
    if (retcode != S_SUCCESS || line_count <= 0)
    {
        printf("PfAuth config load failed!\n");
        return E_ERROR;
    }
    conf.getStringVal("svr_name", _cfg.conn_proper.svr_name);
    conf.getStringVal("cmd_key", _cfg.conn_proper.cmd_key);
	conf.getStringVal("content_type", _cfg.conn_proper.content_type, "application/x-www-form-urlencoded");
    conf.getIntVal("svr_port", &_cfg.conn_proper.svr_port);
    conf.getIntVal("rcv_timeout", &_cfg.conn_proper.rcv_timeout, 5);
    conf.getIntVal("hb_interval", &_cfg.conn_proper.hb_interval, 15);

    conf.getStringVal("log_path", _cfg.log_path, "log");
    conf.getIntVal("log_level", &_cfg.log_level, 4);
    conf.getIntVal("log_file_size", &_cfg.log_file_sz, 8);

#ifdef _BAIDU_
	conf.getStringVal("api_key", _cfg.api_key, "");
	conf.getStringVal("app_secret", _cfg.app_secret, "");
	conf.getStringVal("version", _cfg.version, "1.0");
#endif
    conf.release();
    printf("PfAuth config loaded successfully!\n");

    retcode = _log.initialize(_cfg.log_path, _cfg.log_file_sz*1024, NULL,
        (uos::Logger::LOG_LEVEL)_cfg.log_level);
    if (retcode != S_SUCCESS)
    {
        printf("PfAuth logger initialize failed!\n");
        return E_ERROR;
    }
    printf("PfAuth logger initialized successfully!\n");

    return S_SUCCESS;

}

int PfAuth::release()
{
    int retcode = E_ERROR;
    retcode = _log.release();
    printf("PfAuth logger released!\n");
    _mtx.lock();
    if (_instance != 0)
    {
        delete _instance;
        _instance = 0;
    }
    _mtx.unlock();
    return S_SUCCESS;
}

Connector* PfAuth::createConn()
{
    Connector* conn = NULL;
    ConnProperty& proper = PfAuth::singleton()->_cfg.conn_proper;
    try
    {
        conn = new Connector(proper);
        _log.info("PfAuth::createConn| Create one new connector!\n");
    }
    catch (...)
    {
        _log.info("PfAuth::createConn| Create connector failed!\n");
        if (conn != NULL)
        {
            delete conn;
            conn = NULL;
        }
    }
    return conn;
}

void PfAuth::releaseConn(Connector* conn)
{
    if (conn != NULL)
    {
        delete conn;
    }
    _log.info("PfAuth::releaseConn| release one connector!\n");
}


