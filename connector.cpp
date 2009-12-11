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

Connector::Connector(const ConnProperty& proper)
: _proper(proper)
, _remote_addr(_proper.svr_name, _proper.svr_port)
, _connected(false)
, _last_active_time(0)
, _sequence(SEQ_INIT_VAL)
{
	_thr_id = pthread_self();

    reconnect();
}

Connector::~Connector()
{
    if (true == _connected)
    {
        _sock.close();
    }
}

int Connector::reconnect()
{
    int retcode = E_ERROR;
    int retval  = E_ERROR;

    uint64_t ullOpBgnTime = 0;          // unit is (ms)
    uint64_t ullOpEndTime = 0;          // unit is (ms)

    // if never connected or connection is closed.
    if (false == _connected)
    {
        // create new socket.
        retcode = _sock.socket(AF_INET, SOCK_STREAM, 0);
        if (retcode != S_SUCCESS)
        {
            retval = retcode;
            goto ExitError;
        }

        retcode = _sock.settimeout(_proper.rcv_timeout * 1000);
        if (retcode != S_SUCCESS)
        {
            retval = retcode;
            goto ExitError;
        }
    }

    time(&_last_active_time);
    SysTimeValue::getTickCount(&ullOpBgnTime);
    // connect to remote host.
    retcode = _sock.connect(_remote_addr);
    if (S_SUCCESS == retcode && true == _connected)
    {
        DEBUGLOG("Connector::reconnect| Already connected!\n");
        goto ExitOK;
    }
    else if (retcode != S_SUCCESS)
    {
        DEBUGLOG("Connector::reconnect| connect error:%d\n", retcode);
        disconnect();
        retval = retcode;
        goto ExitError;
    }

    _connected = true;
    SysTimeValue::getTickCount(&ullOpEndTime);
    INFOLOG("Connector::reconnect| Connected to %s:%d. |TC = %llu\n",
        _remote_addr.IPString().c_str(),
        _remote_addr.port(),
        ullOpEndTime - ullOpBgnTime);

ExitOK:
    retval = S_SUCCESS;
ExitError:
    return retval;
}

int Connector::disconnect()
{

    int retcode = E_ERROR;
    struct linger lg;
    lg.l_onoff = 1;
    lg.l_linger = 0;
    retcode = _sock.setsockopt(SOL_SOCKET, SO_LINGER, (char*)&lg, sizeof(lg));
    retcode = _sock.close();

    _connected = false;
    INFOLOG("Connector::disconnect| Connection closed!\n");
    return S_SUCCESS;
}

int Connector::next_seq()
{
    if (++_sequence >= LONG_MAX )
        _sequence = SEQ_INIT_VAL; // recycle use.
    return _sequence;
}

int Connector::userAuth(const char* user_name,
    const char* password,
    const char* game_id)
{
    int retcode = E_ERROR;
    int retval  = E_ERROR;


    uint64_t ullOpBgnTime = 0;          // unit is (ms)
    uint64_t ullOpEndTime = 0;            // unit is (ms)


    SysTimeValue::getTickCount(&ullOpBgnTime);

ExitError:
    SysTimeValue::getTickCount(&ullOpEndTime);
    INFOLOG("PfAuth|%d|%s|%s|GID=%s|TC=%llu\n",
        retval, user_name, password, game_id, ullOpEndTime - ullOpBgnTime);
    return retval;
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
        retcode = reconnect();
        if (retcode != S_SUCCESS)
        {
            retval = retcode;
            goto ExitError;
        }
    }

    if ( (retcode = _sock.send_all(buffer, nbytes)) < 0 )
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

