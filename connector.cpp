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
    // reserved for auto increas.
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
                _conn[i].do_connect();
                if (retcode != S_SUCCESS)
                    continue;
                _conn[i].setState( Connector::S_CONNECTING );
                // select for reading and writing.
                _watch.add_fd( _conn[i].fileno() , FDW_READ );
                _watch.add_fd( _conn[i].fileno() , FDW_WRITE );
            }
            else if ( _conn[i].isDone() && !_conn[i].isTimeout() )
            {
                // get one request.
                retcode = _pfn_req( _conn[i]._wr_buf, &_conn[i]._wr_size, &_conn[i]._sequence );
                if (retcode != S_SUCCESS)
                    continue;
                _conn[i].setState( Connector::S_WRITING );
                _watch.add_fd( _conn[i].fileno(), FDW_WRITE );
            }
            else if ( _conn[i].isConnecting()
                      && ( _watch.check_fd( _conn[i].fileno(), FDW_WRITE ) || _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
                    )
            {
                // continue connect.
                _conn[i].do_connect();
                if (retcode != S_SUCCESS)
                    continue;
                _watch.del_fd( _conn[i].fileno() );
                _conn[i].setState( Connector::S_WRITING );
                _watch.add_fd( _conn[i].fileno(), FDW_WRITE );
            }
            else if ( _conn[i].isReading() && _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
            {
                retcode = _conn[i].do_read();
                if (retcode != S_SUCCESS)
                    continue;
                // TODO: close.
                // set one response while read over.;
                _pfn_res( retcode, _conn[i]._rd_buf, _conn[i]._rd_size, _conn[i]._sequence );
                // reset conn status
                _conn[i]._sequence = 0;
                _watch.del_fd( _conn[i].fileno() );
                _conn[i].setState( Connector::S_DONE );
            }
            else if ( _conn[i].isWriting() && _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
            {
                retcode = _conn[i].do_write();
                if (retcode != S_SUCCESS)
                    continue;
                // if write over, prepare for reading response.
                _conn[i].setState( Connector::S_READING );
                _watch.del_fd( _conn[i].fileno() );
                _watch.add_fd( _conn[i].fileno(), FDW_READ );
            }
            else
            {
                if ( _conn[i].isTimeout() )
                {
                    _conn[i].close();
                    _watch.del_fd( _conn[i].fileno() );
                    _conn[i].setState( Connector::S_FREE );
                    // response to GS.
                    _pfn_res( E_SYS_NET_TIMEOUT, 0, 0, _conn[i]._sequence );
                }
            }
        }// end of for.

    }

}

Connector::~Connector()
{
    if ( ! isFree() )
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

//    _connected = false;
    INFOLOG("Connector::disconnect| Connection closed!\n");
    return S_SUCCESS;
}

int Connector::do_connect()
{
    if ( isFree() )
    {
        Socket::socket(AF_INET, SOCK_STREAM, 0);
        Socket::setblocking(false);
        try
        {
            Socket::connect( uos::SockAddr(_prop.remote_host, _prop.remote_port) );
        }
        catch (...)
        {
            // TODO: 连接调用失败，就关闭释放文件描述符。
            Socket::close();
            setState( S_FREE );
        }
        return S_SUCCESS;
    }
    else if ( isConnecting() )
    {
        int error;
        int n = sizeof(error);
        if ( Socket::getsockopt(SOL_SOCKET, SO_ERROR, &error, &n) < 0 || error != 0 )
        {
            printf("nonblocking connect failed!\n");
            // TODO: deal with connect failed.
        }
        printf("connection established!\n");
        return S_SUCCESS;
    }
}

int Connector::do_read()
{
    int retval  = E_ERROR;
    int msg_len = 0;

    // get msg length.
    int retcode = Socket::recv(_rd_buf + _rd_idx, _rd_size - _rd_idx);
    if ( _rd_idx > 0 && E_SYS_NET_TIMEOUT == retcode ) // EAGAIN
    {
        parse_msg();
    }
    else if (E_SYS_NET_CLOSED == retcode )
    {
    }


    // receive whole msg.


//    *nbytes = msg_len;

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


int Connector::do_write()
{
    int retcode = E_ERROR;
    int retval  = E_ERROR;

    if ( (retcode = Socket::send_all(_wr_buf, _wr_size)) < 0 )
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

