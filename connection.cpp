#include "connector.h"
#include <OperationCode.h>
#include <time.h>
#include "pfauth.h"
#include <SysTimeValue.h>
#include <iostream>
#include <string>


using namespace std;

#define SEQ_INIT_VAL    0xFFFF

#ifdef E_ERROR
#   undef E_ERROR
#   define E_ERROR      -1
#endif

#define INFOLOG PfAuth::singleton()->_log.info
#define DEBUGLOG    PfAuth::singleton()->_log.debug
#define CFGINFO PfAuth::singleton()->_cfg


Connection::Connection(const ConnProp& prop)
{
	_sequence = 0;
	_state = 0;
	_last_active_time = 0;

	_rd_idx = 0;
	_wr_idx = 0;
	_rd_size = 0;
	_wr_size = 0;
	setProp(prop);

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
    INFOLOG("Connection::disconnect| Connection closed!\n");
    return S_SUCCESS;
}

int Connection::do_connect()
{
    int retcode = E_ERROR;
    if ( isFree() )
    {
        Socket::socket(AF_INET, SOCK_STREAM, 0);
        Socket::setblocking(false);
        try
        {
            retcode = Socket::connect( uos::SockAddr(_prop.remote_host, _prop.remote_port) );
            // TODO: deal with connect.
            if (-1  == retcode)
            {
            	return -1;
            }
	        return S_SUCCESS;
        }
        catch (...)
        {
            Socket::close();
            setState( S_FREE );
            return -1;
        }
    }
    else if ( isConnecting() )
    {
        int error;
        int n = sizeof(error);
        if ( Socket::getsockopt(SOL_SOCKET, SO_ERROR, &error, &n) < 0 || error != 0 )
        {
            printf("nonblocking connect failed!\n");
            Socket::close();
            setState( S_FREE );
            return -1;
        }
	    printf("connection established!\n");
    	return S_SUCCESS;
    }

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
    }
    // remote closed, maybe whole msg received.
    else if ( _rd_idx > 0 && 0 == retcode )
    {
        printf("remote closed, maybe whole msg received.\n");
        //parse_msg();
        retval = S_SUCCESS;
    }
    else if (E_SYS_NET_INVALID == retcode)
    {
        setState( S_FREE );
        retval = E_SYS_NET_INVALID;
    }
    else if (retcode < 0 && retcode != E_SYS_NET_TIMEOUT)
    {
        DEBUGLOG("Connection::recvMsg| Detected connection error:%d\n", retcode);
        this->disconnect();
        retval = E_SYS_NET_INVALID;
    }

    return retval;
}


int Connection::do_write()
{
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
    else if ( E_SYS_NET_INVALID == retcode )
    {
        setState( S_FREE );
        retval = E_SYS_NET_INVALID;
    }
    else if (retval < 0 && retval != E_SYS_NET_TIMEOUT)
    {
        DEBUGLOG("Connection::sendMsg| Detected connection error:%d\n", retval);
        this->disconnect();
        retval = E_SYS_NET_INVALID;
    }

    return retval;
}





