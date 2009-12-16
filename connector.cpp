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


//////////////////////////////////////////////////////////////////////////
static char* trimLeft(char* buffer)
{
	ASSERT(buffer != NULL);
	char* pch = buffer;
	for (int i = 0; i < strlen(pch); ++i)
	{
		if (pch[i] != '\t' && pch[i] != ' ')
		{
			strcpy(buffer, pch + i);
			buffer[ strlen(pch) ] = 0;
			break;
		}
	}
	return buffer;

}

static char* trimRight(char* buffer)
{
	ASSERT(buffer != NULL);
	char* pch = buffer;
	for (int i = (strlen(pch) - 1); i >= 0; --i)
	{
		if (pch[i] != '\t' && pch[i] != ' ' && pch[i] != '\n' && pch[i] != '\r')
		{
			buffer[i + 1] = 0;
			break;
		}
	}
    return buffer;
}

static char* trim(char* buffer)
{
	ASSERT(buffer != NULL);
	return trimRight( trimLeft(buffer) );
}


//////////////////////////////////////////////////////////////////////////

Connector::Connector(uint32_t size, const ConnProp& prop)
{
    _conn_size = size;
    uint32_t max = _watch.size_limit();
    _conn_size = (max <= size) ? max : size;
    _conn = new Connection[_conn_size];
    for (uint32_t i=0; i< _conn_size; ++i)
    {
    	_conn[i].setProp(prop);
    }

	_terminate = false;
    // start to run.
    start();
};

Connector::~Connector()
{
	_terminate = true;
	wait();
	delete[] _conn;
}


void Connector::run()
{
    // reserved for auto increas.
    _conn_cnt = _conn_size;
    int nready = 0;
    // main loop
    while ( (! _terminate) || (_conn_cnt > 0) )
    {
        nready = _watch.watch(0);
        if ( nready < 0 )
        {
            if (errno == EINTR || errno == EAGAIN )
                continue;
            printf("watch failed: %d: %s\n", errno, strerror(errno));
            break;
        }

        // if no connection is ready, just test timeout!
        int retcode = -1;
        if ( 0 == nready )
        {
	        for (uint32_t i = 0; i < _conn_cnt; ++i)
	        {
	        	// if some connection is free, load task.
	            if ( _conn[i].isFree() )
	            {
	                retcode = _conn[i].do_connect();
	                if (retcode != S_SUCCESS)
	                    continue;
	                _conn[i].setState( Connection::S_CONNECTING );
	                // select for reading and writing.
	                _watch.add_fd( _conn[i].fileno() , FDW_RDWR );
	            }
	            else if ( _conn[i].isDone() && !_conn[i].isTimeout() )
	            {
	                // get one request.
	                retcode = getRequest( _conn[i]._wr_buf, &_conn[i]._wr_size, &_conn[i]._sequence );
	                if (retcode != S_SUCCESS)
	                    continue;
	                _conn[i].setState( Connection::S_WRITING );
	                _watch.add_fd( _conn[i].fileno(), FDW_WRITE );
	            }
	            else if ( _conn[i].isTimeout() )
                {
                	if ( !_conn[i].isDone() )
                	{
	                    setResponse( E_SYS_NET_TIMEOUT, 0, 0, _conn[i]._sequence );
	                    _watch.del_fd( _conn[i].fileno() );
                	}
                    _conn[i].disconnect();
                }
	        }
	        continue;
        }

        for (uint32_t i = 0; i < _conn_cnt; ++i)
        {
            if ( _conn[i].isFree() )
            {
                retcode = _conn[i].do_connect();
                if (retcode != S_SUCCESS)
                    continue;
                _conn[i].setState( Connection::S_CONNECTING );
                // select for reading and writing.
                _watch.add_fd( _conn[i].fileno() , FDW_RDWR );
            }
            else if ( _conn[i].isDone() && !_conn[i].isTimeout() )
            {
                // get one request.
                retcode = getRequest( _conn[i]._wr_buf, &_conn[i]._wr_size, &_conn[i]._sequence );
                if (retcode != S_SUCCESS)
                    continue;
                _conn[i].setState( Connection::S_WRITING );
                _watch.add_fd( _conn[i].fileno(), FDW_WRITE );
            }
            else if ( _conn[i].isConnecting()
                      && ( _watch.check_fd( _conn[i].fileno(), FDW_WRITE ) || _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
                    )
            {
                // continue connect.
                retcode = _conn[i].do_connect();
                if (S_SUCCESS == retcode)
                {
	                _watch.del_fd( _conn[i].fileno() );
	                // get one request.
	                retcode = getRequest( _conn[i]._wr_buf, &_conn[i]._wr_size, &_conn[i]._sequence );
	                if (S_SUCCESS == retcode)
	                {
		                _conn[i].setState( Connection::S_WRITING );
		                _watch.add_fd( _conn[i].fileno(), FDW_WRITE );
	                }
	                else
	                	_conn[i].setState( Connection::S_DONE );
                }
                else
                {
	                _watch.del_fd( _conn[i].fileno() );
                	_conn[i].disconnect();
                }
            }
            else if ( _conn[i].isReading() && _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
            {
                retcode = _conn[i].do_read();
                if (S_SUCCESS == retcode)
                {
		            // set one response while read over.;
	                setResponse( retcode, _conn[i]._rd_buf, _conn[i]._rd_idx, _conn[i]._sequence );
	                // reset conn status
	                _conn[i]._sequence = 0;
	                _watch.del_fd( _conn[i].fileno() );
	                // work over!!
	                if ( _conn[i].isDone() )
	                {
	                	_conn[i].disconnect();
	                }// else connection is Free status.
                }
                // else if connection closed with error.
                else if ( retcode != E_SYS_NET_TIMEOUT )
                {
	                setResponse( E_SYS_NET_INVALID, 0, 0, _conn[i]._sequence );
	                // reset conn status
	                _conn[i]._sequence = 0;
	                _watch.del_fd( _conn[i].fileno() );
	                _conn[i].disconnect();
                }
                // else continue read.
            }
            else if ( _conn[i].isWriting() && _watch.check_fd( _conn[i].fileno(), FDW_WRITE ) )
            {
                retcode = _conn[i].do_write();
                // if write over, prepare for reading response.
                if (S_SUCCESS == retcode)
                {
	                _conn[i].setState( Connection::S_READING );
	                _watch.del_fd( _conn[i].fileno() );
	                _watch.add_fd( _conn[i].fileno(), FDW_READ );
	            }
                // if socket is not invalid, remove it.
	            else if (E_SYS_NET_INVALID == retcode)
                {
	                setResponse( E_SYS_NET_INVALID, 0, 0, _conn[i]._sequence );
                    _watch.del_fd( _conn[i].fileno() );
                    _conn[i].disconnect();
                }
                // else continue write.
            }
            else
            {
                if ( _conn[i].isTimeout() )
                {
                	printf("socket `%d' is timeout!\n", _conn[i].fileno());
                    // response to GS.
                    setResponse( E_SYS_NET_TIMEOUT, 0, 0, _conn[i]._sequence );
                    _watch.del_fd( _conn[i].fileno() );
                    _conn[i].disconnect();
                }
            }
            //usleep(10);
        }// end of for.
        usleep(10);
        printf("=========================================================================================\n");
    }
}

int Connector::getRequest(char* buffer, uint32_t* nbytes, uint32_t* sequence)
{
	int retval = -1;
	_mtx.lock();
    MsgIter iter = _msg_map.begin();
    while ( !_msg_map.empty() && iter != _msg_map.end() )
    {
        AuthMsg& msg = iter->second;
        if (msg.state == 0)
        {
        	std::string req = ftxy4399_request_encode(msg, _prop.remote_host, _prop.remote_port, "/ftxy4399/userAuth");
        	int len = req.length();
        	if ( *nbytes >= len )
            {
            	*nbytes = len;
	        	strncpy(buffer, req.c_str(), *nbytes);
	        	*sequence = iter->first;
	        	// change state.
	        	msg.state = 1;
	            retval = S_SUCCESS;
	            break;
	        }
	        else
	        {
	        	printf("msg length is too big.\n");
	        	msg.state = 2;
	            msg.retcode = -1; // TODO: msg invalid.
	        }
        }
	    ++iter;
    }
	_mtx.unlock();
	return retval;
}

int Connector::setResponse(int retcode, const char* buffer, uint32_t nbytes, uint32_t sequence)
{
	int retval = -1;
	_mtx.lock();
	MsgIter iter = _msg_map.find(sequence);
    if ( iter != _msg_map.end() )
    {
        AuthMsg& msg = iter->second;
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

int Connector::sendRequest(const AuthMsg& req_msg, uint32_t sequence)
{
	int retval = -1;
	_mtx.lock();
	printf("Connector::sendRequest | map size:%d\n", _msg_map.size());
	if ( _msg_map.size() > 1024 )
	{
    	printf("[no memory], sequence :%d\n", sequence);
	}
	else
	{
		std::pair<MsgIter, bool> ret_pair = _msg_map.insert( MsgPair(sequence, req_msg) );
		AuthMsg& msg = ( ret_pair.first )->second;
		msg.insert_time = time(0);
        retval = S_SUCCESS;
	}
	_mtx.unlock();
	return retval;
}

int Connector::recvResponse(AuthMsg& res_msg, uint32_t* sequence)
{
	int retval = -1;
	_mtx.lock();
    MsgIter iter = _msg_map.begin();
    while (!_msg_map.empty() && iter != _msg_map.end())
    {
        AuthMsg& msg = iter->second;
        //printf("----------msg_seq=%d, msg_state=%d, size=%d\n", iter->first, msg.state, _msg_map.size());
        if (0 == msg.state)
        {
        	// TODO: check timeout.
        	if ( time(0) > msg.insert_time + _prop.timeout_secs )
        	{
        		msg.state = 2;
        		msg.retcode = -302; // TODO: timeout!!!
        	}
        	++iter;
        }
        else if (2 == msg.state)
        {
        	res_msg = msg;
        	*sequence = iter->first;
            _msg_map.erase(iter++);
        	retval = S_SUCCESS;
        	break;
        }
        else
        {
            ++iter;
        }
    }
	_mtx.unlock();
	return retval;
}


std::string ftxy4399_request_encode(const AuthMsg& msg, std::string host, int port, std::string uri)
{
	char buf[CONN_BUF_SZ] = {0};
	std::string content = "user_id="+msg.user_id+"&user_name="+msg.user_name+"&time="+msg.time+"&flag="+msg.flag;
    sprintf( buf, "POST %s HTTP/1.1\r\n" // /ftxy4399/userAuth
		"Host: %s:%d\r\n"
		"Pragma: no-cache\r\n"
		"Accept: */*\r\n"
		"Connection: Close\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"\r\n%s\r\n", uri.c_str(), host.c_str(), port, content.length(), content.c_str() );
	return buf;
}

void ftxy4399_response_decode(std::string res, AuthMsg& msg)
{
    int pos0 = 0;
    int pos1 = 0;
    int pos2 = 0;
    std::string header;
    std::string content;
    std::string body;
    std::string line;
    char content_len[16] = {0};
    int length = 0;
    bool bchunked = false;
    if  (  ( pos1 = res.find("\r\n\r\n") ) != -1 )
    {
        header = res.substr(pos0, pos1 - pos0);
        body = res.substr(pos1 + 4);
    }
    while( ( pos1 = header.find("\r\n", pos0) ) != -1 )
    {
        line = header.substr(pos0, pos1 - pos0);
        if ( ( pos2 = line.find("Transfer-Encoding:") ) != -1 )
        {
            if ( line.find("chunked", pos2) != -1 )
            {
                bchunked = true;
                break;
            }
        }
        else if ( ( pos2 = line.find("Content-Length:") ) != -1 )
        {
            strncpy(content_len, (line.substr(pos2+15)).c_str(), 16);
            length = atoi( trim(content_len) );
            break;
        }
        pos0 = pos1 + 2;
    }
    // parse body.
    if (bchunked)
    {
        int i = 0;
        pos0 = 0;
        while( ( pos1 = body.find("\r\n", pos0) ) != -1 )
        {
            line = body.substr(pos0, pos1 - pos0);
            if (i%2 != 0)
            {
                content += line;
            }
            pos0 = pos1 + 2;
            ++i;
        }
    }
    else
    {
        content = body;
    }
    // parse content.
    printf(">>>>>>>>>>>>>>>>>>>>>> %s\n", content.c_str());
    
	if ( ( pos1 = content.find("|") ) != -1 )
	{
		msg.retcode = atoi( (content.substr(0, pos1)).c_str() );
		msg.adult = atoi( (content.substr(pos1+1)).c_str() );
	}
	else
	{
		msg.retcode = -1;
		msg.adult = 0;
	}
}




