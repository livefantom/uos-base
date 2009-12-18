// Simple text file log writer interface.
// Copyright(C) 2009 LineKong.com
// @file logger.cpp
// @author: Benjamin
// @date: 2009-07-02

#include "logger.h"

#ifdef WIN32
#   include <Windows.h>
#   include <io.h>
#   include <direct.h>
#   include <process.h>
#   define snprintf     _snprintf
#else
#   include <unistd.h>
#   include <pthread.h>
#endif
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>


#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE 4096
#endif


#define MAX_TIME_STR    32

_UOS_BEGIN

static char* strTime(char* buffer, int max_len)
{
    if ( NULL == buffer )
    	return NULL;

    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    max_len = strftime(buffer, max_len, "%Y-%m-%d_%H_%M_%S", timeinfo);
    if (max_len < 19)
    {
        return NULL;
    }
    return buffer;
}

Logger::Logger()
: _file_sz(0)
, _level(LOG_INFO)
, _log_fd(-1)
{
    bzero(_dir, sizeof(_dir));
    bzero(_prefix, sizeof(_prefix));
}

Logger::~Logger()
{
}

int Logger::initialize(
    const char* dir, uint32_t kbytes /* = DEFAULT_FILE_SZ */,
    const char* prefix /* = NULL */, LOG_LEVEL level /* = LOG_INFO */
)
{
    int retval = -1;
    if (dir == NULL)
    {
        printf("invalid directory argument!\n");
        return -1;
    }

    strncpy(_dir, dir, MAX_PATH);
    _dir[MAX_PATH] = '\0';
    _file_sz = kbytes * 1024;
    if (prefix != NULL)
    {
        strncpy(_prefix, prefix, MAX_PREFIX);
        _prefix[MAX_PREFIX] = '\0';
    }
    _level = level;

    // create log directory.
#ifdef WIN32
    retval = mkdir(_dir);
#else
    retval = mkdir(_dir, 0755);
#endif
    if (-1 == retval && errno != EEXIST)
    {
        printf("Create directory %s failed, error: %d: %s\n", _dir, errno, strerror(errno));
        return -1;
    }
    return 1;
}

int Logger::release()
{
    MutexLocker locker(&_log_mtx);
    if (_log_fd != -1)
    {
        close(_log_fd);
        _log_fd = -1;;
    }
    return 1;
}

static uint32_t gettid()
{
#ifdef WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

int Logger::voutput(LOG_LEVEL level, const char* format, va_list ap)
{
    if (NULL == format)
    	return -1;

    if (_level < level)
        return 1;

    int retval = -1;
    int log_len = 0;
    char buffer[MAX_BUFFER_SIZE+1] = {0};
    char time_buf[MAX_TIME_STR+1] = {0};
    struct timeval tv;
    struct tm* timeptr = NULL;
    time_t seconds = 0;
    uint64_t mseconds = 0;

    gettimeofday(&tv, NULL);
    seconds = tv.tv_sec;
    mseconds = tv.tv_usec / 1000;
    timeptr = localtime(&seconds);

    snprintf(time_buf, sizeof(time_buf), "%d-%02d-%02d_%02d:%02d:%02d.%03lld",
        1900 + timeptr->tm_year,
        timeptr->tm_mon+1,
        timeptr->tm_mday,
        timeptr->tm_hour,
        timeptr->tm_min,
        timeptr->tm_sec,
        mseconds);

    time_buf[MAX_TIME_STR] = '\0';
    sprintf(buffer, "%s|%c|%u|%08X|", time_buf, 'U',  getpid(), gettid());
    retval = vsnprintf(buffer+strlen(buffer), MAX_BUFFER_SIZE, format, ap);
    if (retval < 0)
    {
        printf("vsnprintf error: %d: %s\n", errno, strerror(errno));
        return -1;
    }
    buffer[MAX_BUFFER_SIZE] = '\0';

    switch_file();
    // write contents to file.
    log_len = strlen(buffer);
    if ( -1 == (retval = ::write(_log_fd, buffer, log_len)) )
    {
        printf("Write log error: %d: %s\n", errno, strerror(errno));
        return -1;
    }
    else if (retval < log_len)
    {
        printf("Write log partly: %s:%d\n", __FILE__, __LINE__);
    }
    return retval;
}

int Logger::output(LOG_LEVEL level, const char* format, ...)
{
    if (NULL == format)
    	return output(LOG_FATAL, "%s", "log `format' string is NULL!\n");

    va_list arglist;
    va_start(arglist, format);
    int retval = voutput(level, format, arglist);
    va_end(arglist);

    return retval;
}

int Logger::warning(const char* format, ...)
{
    if (NULL == format)
    	return output(LOG_FATAL, "%s", "log `format' string is NULL!\n");

    if (_level < LOG_WARN)
        return 1;

    va_list arglist;
    va_start(arglist, format);
    int retval = voutput(LOG_WARN, format, arglist);
    va_end(arglist);

    return retval;
}

int Logger::info(const char* format, ...)
{
    if (NULL == format)
    	return output(LOG_FATAL, "%s", "log `format' string is NULL!\n");

    if (_level < LOG_INFO)
        return 1;

    va_list arglist;
    va_start(arglist, format);

    int retval = voutput(LOG_INFO, format, arglist);
    va_end(arglist);

    return retval;
}

int Logger::debug(const char* format, ...)
{
    if (NULL == format)
    	return output(LOG_FATAL, "%s", "log `format' string is NULL!\n");

    if (_level < LOG_DEBUG)
        return 1;

    va_list arglist;
    va_start(arglist, format);
    int retval = voutput(LOG_DEBUG, format, arglist);
    va_end(arglist);

    return retval;
}

int Logger::switch_file()
{
    int retval = -1;
    char file_path[MAX_PATH+1] = {0};
    time_t rawtime;
    bool need_switch = false;

    _log_mtx.lock();
    if (_log_fd != -1)
    {
        // judge whether file should be switch.
        struct stat log_stat;
        struct tm stm;
        if ( -1 == fstat(_log_fd, &log_stat) )
        {
            printf("Get file status error: %d: %s\n", errno, strerror(errno));
            // we continue write logs to the old file.
        }
        else
        {
            // compare modification time.
            time(&rawtime);
            int st_day = localtime_r(&log_stat.st_mtime, &stm)->tm_mday;
            int day = localtime_r(&rawtime, &stm)->tm_mday;
            if ( st_day != day )
            {
                need_switch = true;
            }
            // compare file size.
            if (log_stat.st_size >= _file_sz)
            {
                need_switch = true;
            }
        }
    }

    do
    {
        // close old file.
        if (_log_fd != -1 && true == need_switch)
        {
            close(_log_fd);
            _log_fd = -1;
        }
        // create new log file.
        if (-1 == _log_fd)
        {
            // construct log file name.
            char time_buf[MAX_TIME_STR+1] = {0};
            strTime(time_buf, MAX_TIME_STR);
            time_buf[MAX_TIME_STR] = '\0';
            if ( strlen(_prefix) > 0 )
            {
                snprintf(file_path, MAX_PATH, "%s/%s%s.log", _dir, _prefix, time_buf);
            }
            else
            {
                snprintf(file_path, MAX_PATH, "%s/%s.log", _dir, time_buf);
            }
            file_path[MAX_PATH] = '\0';
            // open new log file.
            _log_fd = 
#ifdef WIN32
                open(file_path, O_CREAT | O_APPEND | O_WRONLY, S_IREAD);
#else
                open(file_path, O_CREAT | O_APPEND | O_WRONLY, 0444);
#endif
            if (-1 == _log_fd)
            {
                printf("Open file `%s' error: %d: %s\n", file_path, errno, strerror(errno));
                retval = -1;
                break;
            }
        }
        // else no need switch.
        retval = 1;
    } while (false);
    _log_mtx.unlock();

    return retval;

}



_UOS_END



