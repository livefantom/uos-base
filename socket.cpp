// Light weight low level network interface.
// Only support for IPv4.
// Copyright(C) 2009 LineKong.com
// @file socket.cpp
// @author: Benjamin
// @date: 2009-05-02

#include "socket.h"
#include <OperationCode.h>

#ifdef WIN32
#	include <WS2tcpip.h>
#   include <io.h>
#else
#   include <netdb.h>
#	include <fcntl.h>
#endif


_UOS_BEGIN

const int SOCK_ERR_BASE = -10000;

int Socket::socket(int family /* = AF_INET */, int type /* = SOCK_STREAM */, int proto /* = 0 */)
{
    _family = family;
    _type   = type;
    _proto  = proto;

	if (family != AF_INET)
    {
		printf("we only support IPv4 at present.\n");
        return -1;
    }
    if ( -1 == (_sock_fd = ::socket(_family, _type, _proto)) )
    {
        printf("Create socket failed: %d: %s\n", errno, strerror(errno));
        return (SOCK_ERR_BASE - errno);
    }
    return 1;
}


int Socket::connect(const SockAddr& address)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    struct sockaddr_in svraddr;
	svraddr.sin_family = _family;
	svraddr.sin_addr.s_addr = htonl(address._ip4);
	svraddr.sin_port = htons(address._port);
	if ( -1 == ::connect(_sock_fd, (struct sockaddr*) &svraddr, sizeof(svraddr)) )
	{
        printf("Socket | Connect to %s:%d error: %d: %s\n", long2ip(address._ip4).c_str(), address._port, errno, strerror(errno));
        if (EINPROGRESS == errno)
        {
            return E_SYS_NET_TIMEOUT;
        }
        else if (EISCONN == errno)
        {
            return 1;
        }
        else
        {
            return (SOCK_ERR_BASE - errno);
        }
	}
	return 1;
}

int Socket::bind(const SockAddr& address)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    struct ::sockaddr_in svraddr;
	svraddr.sin_family = _family;
	svraddr.sin_addr.s_addr = htonl(address._ip4);
	svraddr.sin_port = htons(address._port);
	if ( -1 == ::bind(_sock_fd, (struct sockaddr*) &svraddr, sizeof(svraddr)) )
	{
		printf("bind error: %d: %s\n", errno, strerror(errno) );
		return (SOCK_ERR_BASE - errno);
	}
	return 1;
}

int Socket::listen(int backlog /* = SOMAXCONN */)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
	if ( -1 == ::listen(_sock_fd, backlog) )
	{
		printf("Listen failed: %d: %s\n", errno, strerror(errno));
        return (SOCK_ERR_BASE - errno);
	}
	return 1;
}

int Socket::close()
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    if ( -1 == ::close(_sock_fd) )
    {
        printf("Close failed: %d: %s\n", errno, strerror(errno));
        return (SOCK_ERR_BASE - errno);
    }
    _sock_fd = -1;
    return 1;
}

int Socket::shutdown(int how)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    if ( -1 == ::shutdown(_sock_fd, how) )
    {
        printf("Shutdown failed: %d: %s\n", errno, strerror(errno));
        return (SOCK_ERR_BASE - errno);
    }
    return 1;
}

int Socket::getsockopt(int level, int optname, void* optval, int* optlen)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    if ( -1 == ::getsockopt(_sock_fd, level, optname, optval, (socklen_t*)optlen) )
    {
        printf("Get `%d|%d' option failed: %d: %s\n", level, optname, errno, strerror(errno));
        return (SOCK_ERR_BASE - errno);
    }
    return 1;

}

int Socket::setsockopt(int level, int optname, const void* optval, int optlen)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    if ( -1 == ::setsockopt(_sock_fd, level, optname, optval, optlen) )
    {
        printf("Set `%d|%d' option failed: %d: %s\n", level, optname, errno, strerror(errno));
        return (SOCK_ERR_BASE - errno);
    }
    return 1;

}

int Socket::setblocking(bool flag)
{
    int flags, newflags;
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }

    flags = fcntl( _sock_fd, F_GETFL, 0 );
    if (-1 == flags)
    {
    	printf("Get flag failed: %d: %s\n", errno, strerror(errno));
        return (SOCK_ERR_BASE - errno);
    }

    if ( false == flag )
        newflags = flags | (int) O_NDELAY;
    else
        newflags = flags & ~ (int) O_NDELAY;

    if ( newflags != flags )
    {
        flags = fcntl( _sock_fd, F_SETFL, newflags );
	    if (-1 == flags)
	    {
	    	printf("Set `%d' flag failed: %d: %s\n", newflags, errno, strerror(errno));
	        return (SOCK_ERR_BASE - errno);
	    }
    }
    return 1;
}

int Socket::settimeout(int millisecs)
{
    int retcode = -1;
    if ( 0 == millisecs )
    {
        return this->setblocking(false);
    }

    struct timeval tv;
    tv.tv_sec   = millisecs / 1000;
    tv.tv_usec  = (millisecs % 1000) * 1000;

    retcode = this->setsockopt(SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, sizeof(tv));
    if ( retcode != 1 )
    {
        printf("Set receive timeout failed: %d: %s\n", errno, strerror(errno));
        return retcode;
    }

    retcode = this->setsockopt(SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, sizeof(tv));
    if ( retcode != 1 )
    {
        printf("Set send timeout failed: %d: %s\n", errno, strerror(errno));
        return retcode;
    }
    return 1;
}

int Socket::recv(char* buffer, int nbytes, int flags /* = 0 */)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    int nrecv = 0;
    while (true)
    {
        if ( -1 == (nrecv = ::recv(_sock_fd, buffer, nbytes, flags)) )
        {
            printf("Socket | Receive data failed: %d: %s\n", errno, strerror(errno));
            if (EINTR == errno)
            {
                nrecv = 0;
                continue;
            }
            else if (ECONNRESET == errno || ENOTCONN == errno || EPIPE == errno)
            {
                return E_SYS_NET_CLOSED;
            }
            else if (ETIMEDOUT == errno || EAGAIN == errno)
            {
                return E_SYS_NET_TIMEOUT;
            }
            else
            {
                return (SOCK_ERR_BASE - errno);
            }
        }
        else
        {
            return nrecv;
        }
    }
}

int Socket::send(const char* buffer, int nbytes, int flags /* = 0 */)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    int nsend = 0;
    while (true)
    {
        if ( -1 == (nsend = ::send(_sock_fd, buffer, nbytes, flags)) )
        {
            printf("Socket | Send data failed: %d: %s\n", errno, strerror(errno));
            if ( EINTR == errno )
            {
                nsend = 0;
                continue;
            }
            else if (ECONNRESET == errno || ENOTCONN == errno || EPIPE == errno)
            {
                return E_SYS_NET_CLOSED;
            }
            else if (EAGAIN == errno)
            {
                return E_SYS_NET_TIMEOUT;
            }
            else
            {
                return (SOCK_ERR_BASE - errno);
            }
        }
        else
        {
            return nsend;
        }
    }
}

int Socket::recv_into(char* buffer, int nbytes, int flags /* = 0 */)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    int nrecv = 0;
    int nleft = nbytes;
    char* cp = buffer;
    while (nleft > 0)
    {
        if ( -1 == (nrecv = ::recv(_sock_fd, cp, nleft, flags)) )
        {
            printf("Socket | Receive data failed: %d: %s\n", errno, strerror(errno));
            if (EINTR == errno)
            {
                nrecv = 0;
                continue;
            }
            else if (ECONNRESET == errno || ENOTCONN == errno || EPIPE == errno)
            {
                return E_SYS_NET_CLOSED;
            }
            else if (ETIMEDOUT == errno || EAGAIN == errno)
            {
                return E_SYS_NET_TIMEOUT;
            }
            else
            {
                return (SOCK_ERR_BASE - errno);
            }
        }
        else if (0 == nrecv)    // EOF
        {
            if (nleft > 0)  // message broken.
            {
                printf("Socket | Receive data failed: Remote closed!\n");
                return E_SYS_NET_CLOSED;
            }
            break;
        }

        nleft -= nrecv;
        cp += nrecv;
    }
    return (nbytes - nleft);   // bytes received.
}

int Socket::send_all(const char* buffer, int nbytes, int flags /* = 0 */)
{
    if (-1 == _sock_fd)
    {
        return E_SYS_NET_INVALID;
    }
    int nsend = 0;
    int nleft = nbytes;
    const char* cp = buffer;

    while (nleft > 0)
    {
        if ( -1 == (nsend = ::send(_sock_fd, cp, nleft, flags)) )
        {
            printf("Socket | Send data failed: %d: %s\n", errno, strerror(errno));
            if ( EINTR == errno )
            {
                nsend = 0;
                continue;
            }
            else if (ECONNRESET == errno || ENOTCONN == errno || EPIPE == errno)
            {
                return E_SYS_NET_CLOSED;
            }
            else
            {
                return (SOCK_ERR_BASE - errno);
            }
        }
        else if (0 == nsend)
        {
            printf("Send data failed: %s:%d\n", __FILE__, __LINE__);
            break;
        }
        nleft -= nsend;
        cp += nsend;
    }
    return (nbytes - nleft);   // bytes sent.
}


//////////////////////////////////////////////////////////////////////////
// SockAddr members
SockAddr::SockAddr(const char* host_name, int port)
: _port(port)
{
    if (0 == host_name)
    {
        _ip4 = INADDR_ANY;
    }
    else
    {
        struct addrinfo	hints;
        struct addrinfo* ai_list;

        bzero(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (getaddrinfo(host_name, NULL, &hints, &ai_list) != 0)
        {
            throw "invalid `host_name' for resolving.";
        }
        _ip4 = ntohl( ( (sockaddr_in*)ai_list->ai_addr )->sin_addr.s_addr );
        freeaddrinfo(ai_list);
    }
}

std::string SockAddr::IPString() const
{
    return long2ip(_ip4);
}

char* long2ip(char* ip4_string, uint32_t ip4)
{
	assert(ip4_string != 0);
    int ip_num = htonl(ip4);
#ifndef WIN32
	if (inet_ntop(AF_INET, &ip_num, ip4_string, INET_ADDRSTRLEN) <= 0)
	{
	}
#endif
	return ip4_string;
}

std::string long2ip(uint32_t ip4)
{
	char ip4_string[INET_ADDRSTRLEN] = {0};
    int ip_num = htonl(ip4);
#ifndef WIN32
	if (inet_ntop(AF_INET, &ip_num, ip4_string, INET_ADDRSTRLEN) <= 0)
	{
	}
#endif
    return std::string(ip4_string);
}

uint32_t ip2long(const char* ip4_string)
{
	uint32_t ip4;
#ifndef WIN32
	if (inet_pton(AF_INET, ip4_string, &ip4) <= 0)
	{
		throw "invaild `ip_string' for SockAddr construct.";
	}
#endif
	return ntohl(ip4);
}

uint32_t ip2long(const std::string& ip4_string)
{
	uint32_t ip4;
#ifndef WIN32
	if (inet_pton(AF_INET, ip4_string.c_str(), &ip4) <= 0)
	{
		throw "invaild `ip_string' for SockAddr construct.";
	}
#endif
    return ntohl(ip4);
}

_UOS_END


