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

class Socket
{
public:
    Socket():_sock_fd(-1){}
    virtual ~Socket(){}

    // Bind the socket to address.
    int bind(const SockAddr& address);

    // Close the socket.
    int close();

    // Connect to a remote socket at address.
    int connect(const SockAddr& address);

    // Return the socket¡¯s file descriptor(integer).
    int fileno() const
    {
        return _sock_fd;
    }

    // Return the timeout option value in milliseconds of the socket.
    int gettimeout() const;

    // Return the value of the given socket option.
    int getsockopt(int level, int optname, void* optval, int* optlen);

    // Listen for connections made to the socket.
    int listen(int backlog = SOMAXCONN);

    // Receive data.
    int recv(char* buffer, int nbytes, int flags = 0);

    // Receive up to nbytes bytes from the socket, storing the data into buffer.
    int recv_into(char* buffer, int nbytes, int flags = 0);

    // Send data.
    int send(const char* buffer, int nbytes, int flags = 0);

    // Send data to the socket.
    int send_all(const char* buffer, int nbytes, int flags = 0);

    // Set blocking or non-blocking mode of the socket.
    int setblocking(bool flag);

    // Set a timeout on blocking socket operations.
    int settimeout(int millisecs);

    // Set the value of the given socket option.
    int setsockopt(int level, int optname, const void* optval, int optlen);

    // Shut down one or both halves of the connection.
    int shutdown(int how = SHUT_RDWR);

    // Create a new socket using the given family, type and protocol number.
    int socket(int family = AF_INET, int type = SOCK_STREAM, int proto = 0);


protected:
private:
    int _family;
    int _type;
    int _proto;
    int _sock_fd;
};


char* long2ip(char* ip4_string, uint32_t ip4);

std::string long2ip(uint32_t ip4);

uint32_t ip2long(const char* ip4_string);

uint32_t ip2long(const std::string& ip4_string);


//////////////////////////////////////////////////////////////////////////
// class SockAddr
class SockAddr
{
    friend class Socket;
public:
    SockAddr(const char* host_name, int port);
    SockAddr(const std::string& host, int port);
    SockAddr(uint32_t ip4, int port):_ip4(ip4),_port(port){}

    std::string IPString() const;
    int port() const
    {
        return _port;
    }

private:
    uint32_t	_ip4;
    uint16_t	_port;
    uint8_t		_ip6[16];
};

inline SockAddr::SockAddr(const std::string& host, int port)
{
    SockAddr(host.c_str(), port);
}



_UOS_END

#endif//(_UOS_SOCKET_H)

