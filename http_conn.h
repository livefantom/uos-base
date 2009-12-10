// Light weight low level network interface.
// Only support for IPv4.
// Copyright(C) 2009 LineKong.com
// @file socket.h
// @author: Benjamin
// @date: 2009-05-02


#ifndef _UOS_SOCKET_H
#define _UOS_SOCKET_H

#include "uosdef.h"

#ifdef WIN32
#   include <WinSock2.h>
#   define  ECONNRESET      WSAECONNRESET
#   define  ENOTCONN        WSAENOTCONN
#   define  ETIMEDOUT       WSAETIMEDOUT
#   define  EINPROGRESS     WSAEINPROGRESS
#   define  EISCONN         WSAEISCONN
#else
#	include <sys/socket.h>
#	include <arpa/inet.h>
#   define  _close(x)  close(x)
#endif
#include <string>

_UOS_BEGIN

class SockAddr;

class HttpConnection 
{
public:
	enum HTTP_METHOD{
		HTTP_GET,
		HTTP_POST,
		HTTP_PUT,
		HTTP_HEAD
	};

    HttpConnection(const std::string& host, int port, int timeout)
    virtual ~HttpConnection(){}

    // set headerField
    int addHeaderField(const std::string& name, const std::string& value);

    // set request parameter
    int setRequestParameter(const std::string& name, const std::string& value);

    // send request to remote host.
    int request(HTTP_METHOD method = HTTP_POST);

    // get response from remote host.
    std::string getResponse();


protected:
private:
	int _family;
	int _type;
	int _proto;
	int _sock_fd;
};




_UOS_END

#endif//(_UOS_SOCKET_H)

