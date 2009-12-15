#include "connector.h"
#include <OperationCode.h>
#include <time.h>
#include <iostream>
#include <string>


#ifdef E_ERROR
#   undef E_ERROR
#   define E_ERROR      -1
#endif


Connection::Connection(const ConnProp& prop)
{
	Connection();
	setProp(prop);
}

Connection::Connection()
{
	_sequence = 0;
	_state = 0;
	_last_active_time = 0;

	_rd_idx = 0;
	_wr_idx = 0;
	_rd_size = CONN_BUF_SZ;
	_wr_size = CONN_BUF_SZ;

	bzero(_rd_buf, CONN_BUF_SZ);
	bzero(_wr_buf, CONN_BUF_SZ);
}


Connection::~Connection()
{
    if ( ! isFree() )
    {
        disconnect();
    }
}

int Connection::disconnect()
{
    int retcode = E_ERROR;
    struct linger lg;
    lg.l_onoff = 1;
    lg.l_linger = 0;
    retcode = Socket::setsockopt(SOL_SOCKET, SO_LINGER, (char*)&lg, sizeof(lg));
    retcode = Socket::close();

	setState( S_FREE );
    printf("Connection::disconnect| Connection closed!\n");
    return S_SUCCESS;
}

int Connection::do_connect()
{
    int retcode = E_ERROR;
    int retval = E_ERROR;
    if ( isFree() )
    {
        Socket::socket(AF_INET, SOCK_STREAM, 0);
        Socket::setblocking(false);
        try
        {
        	uos::SockAddr addr(_prop.remote_host, _prop.remote_port);
        	retcode = Socket::connect( addr );
			if ( S_SUCCESS == retcode || E_SYS_NET_TIMEOUT == retcode)
	        {
			    retval = S_SUCCESS;
	        }
	        else
	        {
	            Socket::close();
	            setState( S_FREE );
	        }
        }
        catch (...)
        {
            Socket::close();
            setState( S_FREE );
        }
    }
    else if ( isConnecting() )
    {
        int error;
        int n = sizeof(error);
        if ( Socket::getsockopt(SOL_SOCKET, SO_ERROR, &error, &n) < 0 || error != 0 )
        {
            printf("nonblocking connect failed: %d: %s\n", error, strerror(error));
            retval = E_SYS_NET_INVALID;
        }
        else
        {
		    printf("connection established!\n");
	    	retval = S_SUCCESS;
        }
    }
    return retval;
}

int Connection::do_read()
{
    int retval  = E_ERROR;
    int retcode = E_ERROR;

    retcode = Socket::recv(_rd_buf + _rd_idx, _rd_size - _rd_idx);
    if (retcode > 0)
    	_rd_idx += retcode;
    // recevied some msg, try to parse.
    if ( _rd_idx > 0 && E_SYS_NET_TIMEOUT == retcode ) // EAGAIN
    {
    	// TODO: parse msg.
        printf("recevied some msg, try to parse.\n");
        //parse_msg();
        printf("\n%s\n", _rd_buf);
    }
    // remote closed, maybe whole msg received.
    else if ( _rd_idx > 0 && 0 == retcode )
    {
        printf("remote closed, maybe whole msg received.\n");
        //parse_msg();
        
        retval = S_SUCCESS;
    }
    else if (retcode < 0 && retcode != E_SYS_NET_TIMEOUT)
    {
        printf("Connection::do_read| Detected connection error:%d\n", retcode);
        retval = E_SYS_NET_INVALID;
    }

    return retval;
}


int Connection::do_write()
{
	printf("Connection::do_write | fd=%d, wr_size=%d\n", fileno(), _wr_size);
    int retcode = E_ERROR;
    int retval  = E_ERROR;

    retcode = Socket::send(_wr_buf + _wr_idx, _wr_size - _wr_idx);
    if (retcode > 0)
    	_wr_idx += retcode;
    // send over.
    if ( _wr_idx >= _wr_size )
    {
    	retval = S_SUCCESS;
    }
    // send partly, wait for send again.
    else if ( E_SYS_NET_TIMEOUT == retcode )
    {
    	retval = E_SYS_NET_TIMEOUT;
    }
    else if (retval < 0 && retval != E_SYS_NET_TIMEOUT)
    {
        printf("Connection::do_write| Detected connection error:%d\n", retval);
        retval = E_SYS_NET_INVALID;
    }

    return retval;
}





