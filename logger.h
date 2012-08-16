// Simple text file log writer interface.
// Copyright(C) 2009 LineKong.com
// @file logger.h
// @author: Benjamin
// @date: 2009-07-02

#ifndef _UOS_LOGGER_H
#define _UOS_LOGGER_H

#include "uosdef.h"
#include "mutex.h"

#define MAX_PREFIX  30
#define DEFAULT_FILE_SZ 16*1024

#ifndef MAX_PATH
#   define MAX_PATH 510
#endif

_UOS_BEGIN


class Logger
{
public:
    enum LOG_LEVEL
    {
        LOG_FATAL = 1,
        LOG_ERROR,
        LOG_WARN,
        LOG_INFO,
        LOG_DEBUG,
    };

    Logger();
    ~Logger();

    int initialize(
        const char* dir, uint32_t kbytes = DEFAULT_FILE_SZ, const char* prefix = NULL,
        LOG_LEVEL level = LOG_INFO
    );
    int release();

    LOG_LEVEL level() const{ return _level; }
    void setLevel(LOG_LEVEL level){ _level = level; }

    int output(LOG_LEVEL level, const char* format, ...);
    int error(const char* format, ...);
    int warning(const char* format, ...);
    int info(const char* format, ...);
    int debug(const char* format, ...);

protected:
    inline int voutput(LOG_LEVEL level, const char* format, va_list ap);
    inline int switch_file();

private:
    char    _dir[MAX_PATH+1];
    char    _prefix[MAX_PREFIX+1];
    int     _file_sz;
    LOG_LEVEL   _level;

    Mutex   _log_mtx;
    int     _log_fd;

};


_UOS_END

#endif//(_UOS_LOGGER_H)



