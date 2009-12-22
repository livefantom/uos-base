#include "connector.h"
#include "OperationCode.h"
#include "pfauth.h"
#include <sys/time.h>
#include <string>
#include <cstdlib>


#ifdef E_ERROR
#   undef E_ERROR
#   define E_ERROR      -1
#endif

#define INFOLOG	(PfAuth::singleton()->logger()).info
#define DEBUGLOG	(PfAuth::singleton()->logger()).debug


//////////////////////////////////////////////////////////////////////////
static char* trimLeft(char* buffer)
{
    if (buffer == NULL)
        return NULL;

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
    if (buffer == NULL)
        return NULL;

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
    if (buffer == NULL)
        return NULL;

    return trimRight( trimLeft(buffer) );
}

static uint64_t getmillisecs()
{
    struct timeval tv;
    (void) gettimeofday( &tv, (struct timezone*) 0 );
    return (uint64_t)(tv.tv_sec) * 1000L + tv.tv_usec / 1000L;
}
//////////////////////////////////////////////////////////////////////////

Connector::Connector(uint32_t size, uint32_t queue_max, const ConnProp& prop)
        : _prop(prop)
        , _conn_size(size)
        , _queue_max(queue_max)
{
    _conn_cnt = 0;
    uint32_t max = _watch.size_limit();
    _conn_size = (max <= size) ? max : size;
    _conn = new Connection[_conn_size];
    for (uint32_t i=0; i< _conn_size; ++i)
    {
        _conn[i].setProp(_prop);
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
    int nready = 0;
    uint64_t msecs = getmillisecs();
    //while ( (! _terminate) || (_conn_cnt > 0) )
    while ( ! _terminate )
    {
        nready = _watch.watch(0);
        if ( nready < 0 )
        {
            if (errno == EINTR || errno == EAGAIN )
                continue;
            DEBUGLOG("Connector::run | watch failed: %d: %s\n", errno, strerror(errno));
            break;
        }

        //msecs = getmillisecs();
        // if no connection is ready, just test timeout!
        int retcode = -1;
        if ( 0 == nready )
        {
            for (uint32_t i = 0; i < _conn_size; ++i)
            {
                if ( _terminate )
                    break;

                // if some connection is free, load task.
                if ( _conn[i].isFree() )
                {
                    retcode = _conn[i].do_connect();
                    if (retcode != S_SUCCESS)
                        continue;
                    _conn[i].setState( Connection::S_CONNECTING );
                    // select for reading and writing.
                    _watch.add_fd( _conn[i].fileno() , FDW_RDWR );
                    ++_conn_cnt;
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
                    --_conn_cnt;
                }
                usleep(10);
            }// end of for.
        }
        else
        {
            for (uint32_t i = 0; i < _conn_size; ++i)
            {
                if ( _terminate )
                    break;

                if ( _conn[i].isFree() )
                {
                    retcode = _conn[i].do_connect();
                    if (retcode != S_SUCCESS)
                        continue;
                    _conn[i].setState( Connection::S_CONNECTING );
                    _watch.add_fd( _conn[i].fileno() , FDW_RDWR );
                    ++_conn_cnt;
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
                        --_conn_cnt;
                    }
                }
                else if ( _conn[i].isReading() && _watch.check_fd( _conn[i].fileno(), FDW_READ ) )
                {
                    retcode = _conn[i].do_read();
                    if (S_SUCCESS == retcode)
                    {
                        // set one response while read over.
                        setResponse( retcode, _conn[i]._rd_buf, _conn[i]._rd_idx, _conn[i]._sequence );
                        _watch.del_fd( _conn[i].fileno() );
                        {	// ugly codes.
							_conn[i]._rd_idx = 0;
							_conn[i]._wr_idx = 0;
							_conn[i]._rd_size = CONN_BUF_SZ;
							_conn[i]._wr_size = CONN_BUF_SZ;
							bzero(_conn[i]._rd_buf, CONN_BUF_SZ);
							bzero(_conn[i]._wr_buf, CONN_BUF_SZ);
                        }
                        // close socket while short-connection mode.
                        if ( _conn[i].isDone() )// TODO: 
                        {
                            _conn[i].disconnect();
                            --_conn_cnt;
                        }
                        else
                            _conn[i].setState( Connection::S_DONE );
                    }
                    // else if connection closed with error.
                    else if ( retcode != E_SYS_NET_TIMEOUT )
                    {
                    	// TODO: msg resend optmize.
                        setResponse( E_SYS_NET_INVALID, 0, 0, _conn[i]._sequence );
                        // reset conn status
                        _watch.del_fd( _conn[i].fileno() );
                        _conn[i].disconnect();
                        --_conn_cnt;
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
                        --_conn_cnt;
                    }
                    // else continue write.
                }
                else if ( _conn[i].isTimeout() )
                {
                    if ( !_conn[i].isDone() )
                    {
                        DEBUGLOG("Connector::run | Socket `%d' with msg[%d] was timeout!\n", _conn[i].fileno(), _conn[i]._sequence);
                        // report timeout to GS.
                        setResponse( E_SYS_NET_TIMEOUT, 0, 0, _conn[i]._sequence );
                        _watch.del_fd( _conn[i].fileno() );
                    }
                    _conn[i].disconnect();	// TODO: deal with keep-alive state.
                    --_conn_cnt;
                }

                usleep(10);
            }// end of for.
        }

        usleep(100);
        if ( getmillisecs() > msecs + 5000L )
        {
            DEBUGLOG("Connector::run | STAT_INFO: nready=%d, conn_cnt=%d, map_size=%d\n", nready, _conn_cnt, _msg_map.size());
            msecs = getmillisecs();
        }
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
            std::string req = http_request_encode(msg, _prop);
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
                DEBUGLOG( "Connector::getRequest | String of request[%d] is too long!\n%s\n", iter->first, req.c_str() );
                msg.state = 2;
                msg.retcode = E_SYS_NET_NOT_ENOUGH_BUFFER;	// -309
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
            http_response_decode(buffer, msg);
        }
        else
        {
            msg.retcode = retcode;
        }
        msg.state = 2;
        retval = S_SUCCESS;
        INFOLOG("PfAuth|%d|User_ID=%s|User_Name=%s|Password=%s|Time=%s|Flag=%s|Adult=%d|Sequence=%d|TC=%llu\n",
                msg.retcode,
                msg.user_id.c_str(),
                msg.user_name.c_str(),
                msg.password.c_str(),
                msg.time.c_str(),
                msg.flag.c_str(),
                msg.adult,
                sequence,
                getmillisecs() - msg.insert_time
               );
    }
    _mtx.unlock();
    return retval;
}

int Connector::sendRequest(const AuthMsg& req_msg, uint32_t sequence)
{
    int retval = -1;
    _mtx.lock();
    int size = _msg_map.size();
    //printf("Connector::sendRequest | map size:%d\n", );
    if ( _msg_map.size() > _queue_max )
    {
        DEBUGLOG("Connector::sendRequest| Message queue is full, size=%d.\n", size);
        retval = E_SYS_MSG_QUEUE_FULL; // -313
    }
    else
    {
        std::pair<MsgIter, bool> ret_pair = _msg_map.insert( MsgPair(sequence, req_msg) );
        AuthMsg& msg = ( ret_pair.first )->second;
        msg.insert_time = getmillisecs();
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
            if ( getmillisecs() > msg.insert_time + _prop.timeout_secs * 2000L )
            {
                msg.state = 2;
                msg.retcode = E_SYS_MSG_QUEUE_TIMEOUT;	// -314
                INFOLOG("PfAuth|%d|User_ID=%s|User_Name=%s|Password=%s|Time=%s|Flag=%s|Adult=%d|Sequence=%d|TC=%llu\n",
                        msg.retcode,
                        msg.user_id.c_str(),
                        msg.user_name.c_str(),
                        msg.password.c_str(),
                        msg.time.c_str(),
                        msg.flag.c_str(),
                        msg.adult,
                        iter->first,
                        getmillisecs() - msg.insert_time
                       );
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


std::string http_request_encode(const AuthMsg& msg, const ConnProp& prop)
{
    char buf[CONN_BUF_SZ] = {0};
    std::string content = msg.encodeRequest();
    std::string connection = (prop.keep_alive) ? "Keep-Alive" : "Close";

    sprintf( buf, "POST %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Pragma: no-cache\r\n"
             "Accept: */*\r\n"
             "Connection: %s\r\n"
             "Content-Length: %d\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "\r\n%s\r\n",
     		 prop.http_uri,
    		 prop.remote_host, prop.remote_port,
    		 connection.c_str(),
    		 content.length(),
    		 content.c_str()
    );
    return buf;
}

bool try_parse_http_response(std::string res)
{
    int pos0 = 0;
    int pos1 = 0;
    int pos2 = 0;
    std::string header;
    std::string content;
    std::string body;
    std::string line;
    char len[32] = {0};
    int content_length = 0;
    bool bchunked = false;
    if  (  ( pos1 = res.find("\r\n\r\n") ) != -1 )
    {
        header = res.substr(pos0, pos1 - pos0);
        body = res.substr(pos1 + 4);
    }
    while ( ( pos1 = header.find("\r\n", pos0) ) != -1 )
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
            strncpy(len, ( line.substr(pos2+15) ).c_str(), 32);
            content_length = atoi( trim(len) );
            break;
        }
        pos0 = pos1 + 2;
    }
    // parse body.
    if (bchunked)
    {
        int i = 0;
        pos0 = 0;
        while ( ( pos1 = body.find("\r\n", pos0) ) != -1 )
        {
            line = body.substr(pos0, pos1 - pos0);
            if (i%2 != 0)
            {
                content += line;
            }
            else
            {
                content_length += atoi( line.c_str() );
            }
            pos0 = pos1 + 2;
            ++i;
        }
    }
    else
    {
        content = body;
    }
    if ( content.length() == content_length )
        return true;
    else
        return false;

}


void http_response_decode(std::string res, AuthMsg& msg)
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
    while ( ( pos1 = header.find("\r\n", pos0) ) != -1 )
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
        while ( ( pos1 = body.find("\r\n", pos0) ) != -1 )
        {
            line = body.substr(pos0, pos1 - pos0);
            if (i%2 != 0)
            {
                content += line;
            }
            else
            {
                length += atoi( line.c_str() );
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

    msg.decodeResponse(content);
}




