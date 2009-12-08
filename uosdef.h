//////////////////////////////////////////////////////////////////////////
// uosdef.h
// Author: Benjamin
// Date: 2009-05-02

#ifndef _UOS_DEF_H
#define _UOS_DEF_H


#if defined(__cplusplus)
#define _UOS_BEGIN	namespace uos {
#define _UOS_END		}
#endif

//////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
//#	include <crtdefs.h>
#	define SHUT_RD     SD_RECEIVE
#	define SHUT_WR     SD_SEND
#	define SHUT_RDWR   SD_BOTH

typedef unsigned long long  uint64_t;
typedef unsigned int        uint32_t;
typedef unsigned short      uint16_t;
typedef unsigned char       uint8_t;

#   define  bzero(x, y)     memset((x), 0, (y))

#else
#   include <netinet/in.h>
#   define     MAX_PATH    500
#endif



/////////////////////////////
#define DISABLE_COPY(Class) \
    Class(const Class &); \
    Class &operator=(const Class &);


//////////////////////////////////////////////////////////////////////////


#endif//(_UOS_DEF_H)



