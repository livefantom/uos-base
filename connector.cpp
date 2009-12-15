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


