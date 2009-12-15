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
                retcode = getRequest( _conn[i]._wr_buf, _conn[i]._wr_size, &_conn[i]._sequence );
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
                if (S_SUCCESS == retcode)
                {
		            // set one response while read over.;
	                setResponse( retcode, _conn[i]._rd_buf, _conn[i]._rd_size, _conn[i]._sequence );
	                // reset conn status
	                _conn[i]._sequence = 0;
	                _watch.del_fd( _conn[i].fileno() );
	                // work over!!
	                if ( _conn[i].isReading() )
	                {
		                _conn[i].setState( Connector::S_DONE );
	                }// else connection is Free status.
                }
                // else if connection closed with error.
                else if ( _conn[i].isFree() )
                {
	                setResponse( E_SYS_NET_INVALID, 0, 0, _conn[i]._sequence );
	                // reset conn status
	                _conn[i]._sequence = 0;
	                _watch.del_fd( _conn[i].fileno() );
                }
                // else continue read.
            }
            else if ( _conn[i].isWriting() && _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
            {
                retcode = _conn[i].do_write();
                if (retcode != S_SUCCESS)
                    continue;
                // if write over, prepare for reading response.
                if (S_SUCCESS == retcode)
                {
	                _conn[i].setState( Connector::S_READING );
	                _watch.del_fd( _conn[i].fileno() );
	                _watch.add_fd( _conn[i].fileno(), FDW_READ );
	            }
                // else if connection closed with error.
                else if ( _conn[i].isFree() )
                {
	                setResponse( E_SYS_NET_INVALID, 0, 0, _conn[i]._sequence );
	                // reset conn status
	                _conn[i]._sequence = 0;
	                _watch.del_fd( _conn[i].fileno() );
                }
                // else continue write.
            }
            else
            {
                if ( _conn[i].isTimeout() )
                {
                    // response to GS.
                    setResponse( E_SYS_NET_TIMEOUT, 0, 0, _conn[i]._sequence );
                    _conn[i].close();
                    _watch.del_fd( _conn[i].fileno() );
                    _conn[i].setState( Connector::S_FREE );
                }
            }
        }// end of for.

    }

}

int ConnPool::getRequest(char* buffer, uint32_t nbytes, uint32_t* sequence)
{
	int retval = -1;
	_mtx.lock();
    MsgIter iter = _msg_map.begin();
    while ( !_msg_map.empty() && iter != _msg_map.end() )
    {
        AuthMsg msg = iter->second;
        if (msg.state == 0)
        {
        	std::string req = ftxy4399_request_encode(msg);
        	if ( nbytes >= req.length() )
            {
	        	strncpy(buffer, req.c_str(), nbytes);
	        	*sequence = iter->first;
	        	// change state.
	        	msg.state = 1;
	            ++iter;
	            retval = S_SUCCESS;
	        }
        }
    }
	_mtx.unlock();
	return retval;
}

int ConnPool::setResponse(int retcode, const char* buffer, uint32_t nbytes, uint32_t sequence)
{
	int retval = -1;
	_mtx.lock();
	MsgIter iter = _msg_map.find(sequence);
    if ( iter != _msg_map.end() )
    {
        AuthMsg msg = iter->second;
        if (S_SUCCESS == retcode)
        {
        	ftxy4399_response_decode(buffer, msg);
        }
        else
        {
        	msg.retcode = retcode;
        }
        msg.state = 2;
        retval = S_SUCCESS;
	}
	_mtx.unlock();
	return retval;
}

int ConnPool::sendRequest(const AuthMsg& req_msg, uint32_t sequence)
{
	int retval = -1;
	_mtx.lock();
	if ( _msg_map.size() > 1024 )
	{
    	printf("sent msg reponse to GS[no memory], sequence :%d", sequence);
    	retval = -1;// TODO: no memory.
	}
	else
	{
		_msg_map.insert( MsgPair(sequence, req_msg) );
        retval = S_SUCCESS;
	}
	_mtx.unlock();
	return retval;
}

int ConnPool::recvResponse(AuthMsg& res_msg, uint32_t* sequence)
{
	int retval = -1;
	_mtx.lock();
    MsgIter iter = _msg_map.begin();
    while (!_msg_map.empty() && iter != _msg_map.end())
    {
        AuthMsg msg = iter->second;
        if (0 == msg.state)
        {
        	// TODO: check timeout.
        	;
        }
        else if (2 == msg.state)
        {
        	printf("sent msg reponse to GS[OK], sequence :%d", iter->first);
        	res_msg = msg;
        	*sequence = iter->first;
            _msg_map.erase(iter++);
        	retval = S_SUCCESS;
        }
        else
        {
            ++iter;
        }
    }
	_mtx.unlock();
	return retval;
}





Connector::Connector(const ConnProp& prop)
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


Connector::~Connector()
{
    if ( ! isFree() )
    {
        disconnect();
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

	setState( S_FREE );
    INFOLOG("Connector::disconnect| Connection closed!\n");
    return S_SUCCESS;
}

int Connector::do_connect()
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

int Connector::do_read()
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
        DEBUGLOG("Connector::recvMsg| Detected connection error:%d\n", retcode);
        this->disconnect();
        retval = E_SYS_NET_INVALID;
    }

    return retval;
}


int Connector::do_write()
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
        DEBUGLOG("Connector::sendMsg| Detected connection error:%d\n", retval);
        this->disconnect();
        retval = E_SYS_NET_INVALID;
    }

    return retval;
}



std::string ftxy4399_request_encode(const AuthMsg& msg)
{
	char buf[32] = {0};
	std::string content = "user_id="+msg.user_id+"&user_name="+msg.user_name+"&time="+msg.time+"&flag="+msg.flag;
	sprintf(buf, "%d", content.length());

    return "POST /api/ftxy/passport.php HTTP/1.1\r\n"
		"Host: web.4399.com\r\n"
		"Pragma: no-cache\r\n"
		"Accept: */*\r\n"
		"Content-Length: " + std::string(buf) + "\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"\r\n" + content + "\r\n";
}

void ftxy4399_response_decode(std::string res, AuthMsg& msg)
{
	// TODO: parse response.
	msg.retcode = 1;
	msg.adult = 1;
    return;
}


