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


void ConnPool::run()
{
	_conn_cnt = _conn_size;
	int nready = 0;
 	// main loop
	while ( (! _terminate) || (_conn_cnt > 0) )
	{
		nready = _watch.watch(-1);
		if ( nready < 0 )
		{
			if (errno == EINTR || errno == EAGAIN )
				continue;
			printf("watch failed!\n");
			break;
		}
		// if no socket is ready, just test timeout!
		if ( 0 == nready )
		{
			// TODO: check timeout!!!
			continue;
		}

		int retcode = -1;
		for (uint32_t i = 0; i < _conn_cnt; ++i)
		{
			if ( _conn[i].isFree() )
			{
				_conn[i].socket(AF_INET, SOCK_STREAM, 0);
    			_conn[i].setblocking(false);
    			try
    			{
	    			_conn[i].connect( uos::SockAddr(_prop.remote_host, _prop.remote_port) );
    			}
    			catch (...)
    			{
    				;
    			}
				_conn[i].setFlag( Connector::S_CONNECTING );
    			// select for reading and writing.
    			_watch.add_fd( _conn[i].fileno() , FDW_READ );
    			_watch.add_fd( _conn[i].fileno() , FDW_WRITE );
			}
			else if ( _conn[i].isDone() && true != _conn[i].timeout() )
			{
				// TODO: req_func();
				retcode = _pfn_req( _conn[i]._wr_buf, &_conn[i]._wr_size, &_conn[i]._sequence );
				if (retcode != 1)
					continue;
				_conn[i].setFlag( Connector::S_WRITING );
    			_watch.add_fd( _conn[i].fileno(), FDW_WRITE );
			}
			else if ( _conn[i].isConnecting()
				&& ( _watch.check_fd( _conn[i].fileno(), FDW_WRITE ) || _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
		    )
			{
				int error;
				int n = sizeof(error);
				if ( _conn[i].getsockopt(SOL_SOCKET, SO_ERROR, &error, &n) < 0 || error != 0 )
				{
					printf("nonblocking connect failed!\n");
				}
				printf("connection established!\n");
				_watch.del_fd(_conn[i].fileno());
				_conn[i].setFlag( Connector::S_WRITING );
				_watch.add_fd( _conn[i].fileno(), FDW_WRITE );
			}
			else if ( _conn[i].isReading() && _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
			{
				retcode = _conn[i].do_read();
				if (retcode != 1)
					continue;
				// if read over & not close.
				// TODO: res_func();
				_pfn_res( retcode, _conn[i]._rd_buf, _conn[i]._rd_size, _conn[i]._sequence );
				_conn[i]._sequence = 0;
				_watch.del_fd(_conn[i].fileno());
				_conn[i].setFlag( Connector::S_DONE );

				// get another request.
				// TODO: req_func();
				//if (ret != 1)
				//	continue;

			}
			else if ( _conn[i].isWriting() && _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
			{
				retcode = _conn[i].do_write();
				if (retcode != 1)
					continue;
				// if write over!
				_watch.del_fd(_conn[i].fileno());
				_conn[i].setFlag( Connector::S_READING );
				// prepare for reading response.
				_watch.add_fd( _conn[i].fileno(), FDW_READ );
			}
			else
			{
				if ( true == _conn[i].timeout() )
				{
					_conn[i].close();
					_watch.del_fd(_conn[i].fileno());
					_conn[i].setFlag( Connector::S_FREE );
					// TODO: res_func();
				}
			}
		}// end of for.


	}

}



Connector::~Connector()
{
    if (true == _connected)
    {
        Socket::close();
    }
}

int Connector::disconnect()
{

    int retcode = E_ERROR;
    struct linger lg;
    lg.l_onoff = 1;
    lg.l_linger = 0;
    retcode = Socket::setsockopt(SOL_SOCKET, SO_LINGER, (char*)&lg, sizeof(lg));
    retcode = Socket::close();

    _connected = false;
    INFOLOG("Connector::disconnect| Connection closed!\n");
    return S_SUCCESS;
}

int Connector::recvMsg(char* buffer, int* nbytes)
{
    int retval  = E_ERROR;
    int nrecv   = 0;
    int msg_len = 0;

    if (NULL == buffer || NULL == nbytes)
    {
        DEBUGLOG("Connector::recvMsg| Parameters invalid!\n");
        return E_PARAMETER_ERROR;
    }

    if (false == _connected)
    {
        DEBUGLOG("Connector::recvMsg| we havn't connected to remote!\n");
        goto ExitError;
    }

    // receive msg length.


    // get msg length.


    // receive whole msg.


    *nbytes = msg_len;

    retval = S_SUCCESS;
ExitError:
    if (retval < 0 && retval != E_SYS_NET_TIMEOUT)
    {
        DEBUGLOG("Connector::recvMsg| Detected connection error:%d\n", retval);
        this->disconnect();
    }
    if (E_SYS_NET_INVALID == retval)
        _connected = false;

    return retval;
}


int Connector::sendMsg(const char* buffer, int nbytes)
{
    int retcode = E_ERROR;
    int retval  = E_ERROR;

    if (false == _connected)
    {
//        retcode = reconnect();
        if (retcode != S_SUCCESS)
        {
            retval = retcode;
            goto ExitError;
        }
    }

    if ( (retcode = Socket::send_all(buffer, nbytes)) < 0 )
    {
        DEBUGLOG("Connector::sendMsg| error:%d\n", retcode);
        retval = retcode;
        goto ExitError;
    }
    else if (retcode != nbytes) // unreasonable error.
    {
        DEBUGLOG("Connector::sendMsg| NOT all msg content sent!\n");
        retval = -1;
        goto ExitError;
    }

    // record active time.
    time(&_last_active_time);
    retval = S_SUCCESS;
ExitError:
    if (retval < 0 && retval != E_SYS_NET_TIMEOUT)
    {
        DEBUGLOG("Connector::sendMsg| Detected connection error:%d\n", retval);
        this->disconnect();
    }
    if (E_SYS_NET_INVALID == retval)
        _connected = false;

    return retval;
}

