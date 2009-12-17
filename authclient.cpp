// Third-party platform access Authentication module.
// Copyright(C) 2009 LineKong.com
// @file authclient.cpp
// @author: Benjamin
// @date: 2009-06-02

#include "authclient.h"
#include "pfauth.h"
#include <OperationCode.h>
#include <AGIP.h>
#include <time.h>
#include <WinSock2.h>


//#define AUTHLOG (PfAuth::getSingleton()->logger())
#define TIME_KEEP_ALIVE 15
#define  _ERR_SEQ   0

#ifdef WIN32
#else
#include <sys/time.h>
static uint64_t GetTickCount()
{
    struct timeval tmval;
    int nRetCode = gettimeofday(&tmval, NULL);
    if (nRetCode != 0)
    {
        return 0;
    }
    return ((uint64_t)tmval.tv_sec) * 1000 + tmval.tv_usec / 1000;
}
#endif

AuthClient::AuthClient()
: _remote_addr((char*)0, 0)
{
}

AuthClient::~AuthClient()
{
}

int AuthClient::setRequest(const AuthReq& req)
{
    MsgInfo msg;
    msg.req = req;
    msg.req_msec = GetTickCount();

    _list_mtx.lock();

    _msg_list.insert( MsgPair(req.seq, msg) );
    // TODO:add error process

    _list_mtx.unlock();
    return 1;
}

int AuthClient::getResponse(AuthRes& res, int sequence)
{
    _list_mtx.lock();

    MsgIter iter = _msg_list.find(sequence);
    // TODO:add error process
    res = (iter->second).res;
    _msg_list.erase(iter);

    _list_mtx.unlock();
    return 1;
}

void AuthClient::run()
{
    int retcode = -1;

    int sock_fd = _sock.fileno();
    fd_set rd_set;
    fd_set wr_set;
    fd_set ex_set;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    // TODO: add heartbeat msg to queue.

    // main loop.
    while (true)
    {
        FD_CLR( sock_fd, &rd_set );
        FD_CLR( sock_fd, &wr_set );
        FD_CLR( sock_fd, &ex_set );

        FD_SET( sock_fd, &rd_set );
        FD_SET( sock_fd, &wr_set );
        FD_SET( sock_fd, &ex_set );

        if ( -1 == select(sock_fd + 1, &rd_set, &wr_set, &ex_set, &timeout) )
        {
            continue;
        }

        if ( FD_ISSET(sock_fd, &ex_set) )
        {
            FD_CLR(sock_fd, &rd_set);
            FD_CLR(sock_fd, &wr_set);
            FD_CLR(sock_fd, &ex_set);

            onError();
            continue;
        }

        if ( FD_ISSET(sock_fd, &rd_set) )
        {
            onRecv();
        }

        if ( FD_ISSET(sock_fd, &wr_set) )
        {
            onSend();
        }
    }
}

void AuthClient::onRecv()
{
    int retcode = -1;
    int retval = -1;

    char buffer[BUFSIZ];
    int len = 0;
    retcode = recvMsg(buffer, &len);
    if (retcode != S_SUCCESS)
    {
        return;  // recv error, goto select.
    }

    AuthRes res;
    retcode = deserialize(&res, buffer, len);
    int res_req = res.seq;

    MsgIter iter = _msg_list.begin();
    while (!_msg_list.empty() && iter != _msg_list.end())
    {
        uint64_t cur_msec = GetTickCount();
        int req_seq = iter->first;
        MsgInfo msg = iter->second;
        if (msg.status != 3)
        {
            if (cur_msec > msg.req_msec + _timeout*1000) // timeout
            {
                msg.status = 3;
                PfAuth::getSingleton()->innerCallback(-301, NULL, req_seq);
            }
        }

        // already sent and response received.
        if (msg.status == 1 && res_req == req_seq)
        {
            msg.res = res;
            msg.status = 2;
            PfAuth::getSingleton()->innerCallback(1, &msg.res, req_seq);
        }
        ++iter;
    }    

}

void AuthClient::onSend()
{
    int retcode = -1;
    MsgIter iter = _msg_list.begin();
    while (!_msg_list.empty() && iter != _msg_list.end())
    {
        uint64_t cur_msec = GetTickCount();
        int req_seq = iter->first;
        MsgInfo msg = iter->second;
        if (msg.status != 3)
        {
            if (cur_msec > msg.req_msec + _timeout*1000) // timeout
            {
                msg.status = 3;
                PfAuth::getSingleton()->innerCallback(-301, NULL, req_seq);
            }
        }

        if (msg.status == 0) // new msg.
        {
            char buffer[BUFSIZ];
            int len = BUFSIZ;
            retcode = serialize(buffer, &len, &msg.req);
            retcode = sendMsg(buffer, len);
            if (retcode != S_SUCCESS)
            {
                break;  // send error, goto select.
            }
        }
        ++iter;
    }    
}

int AuthClient::serialize(char* pdu, int* nbytes, void* obj)
{
    int retcode = -1;
    int retval = -1;

    if (pdu == NULL || nbytes == NULL || obj == NULL)
    {
        return -1;
    }

    AGIPPlatformAuthen     pfAuthen('<','>');
    AuthReq* req = (AuthReq*)obj;

    // prepare request data.
    pfAuthen.setParameter("UN", req->user_name);
    pfAuthen.setParameter("PW", req->password);
    pfAuthen.setParameter("GID", req->game_id);
    pfAuthen.setSequenceID( _ERR_SEQ );

    retcode = pfAuthen.getDecodedPDU(pdu, AGIP_MAX_BUFFER_SIZE, nbytes);
    if (retcode != S_SUCCESS)
    {
        printf("Construct PDU error! %s:%d.\n", __FILE__, __LINE__);
        retval = retcode;
        return retval;
    }
}

int AuthClient::deserialize(void* obj, char* pdu, int nbytes)
{
    int retcode = -1;
    int retval = -1;

    if (pdu == NULL || nbytes == NULL || obj == NULL)
    {
        return -1;
    }

    AGIPPlatformAuthenRes  pfAuthenRes('<','>');
    AuthRes* res = (AuthRes*)obj;

    retcode = pfAuthenRes.setDecodedPDU(pdu, nbytes, 1);
    if (retcode != S_SUCCESS)
    {
        printf("Parse PDU error! %s:%d.\n", __FILE__, __LINE__);
        retval = retcode;
        return retval;
    }
    retcode = pfAuthenRes.getSequenceID(&res->seq);
    retcode = pfAuthenRes.getParameter("Result", res->result, AGIP_INTEGER_STR_LEN);
}

int AuthClient::reconnect()
{
    int retcode = E_ERROR;
    int retval  = E_ERROR;

    if (true == _connected)
    {
        printf("No need to reconnect!\n");
        goto ExitError;
    }

    // create new socket, and reconnect.
    retcode = _sock.socket(AF_INET, SOCK_STREAM, 0);
    if (retcode != S_SUCCESS)
    {
        retval = retcode;
        goto ExitError;
    }

    retcode = _sock.setblocking(false);
    if (retcode != S_SUCCESS)
    {
        retval = retcode;
        goto ExitError;
    }

    retcode = _sock.connect(_remote_addr);
    if (retcode != S_SUCCESS)
    {
        disconnect();
        retval = retcode;
        goto ExitError;
    }
    printf("Connected to %s:%d.\n", _remote_addr.IPString().c_str(), _remote_addr.port());
    //AUTHLOG.info("Connected to %s:%d.\n", _remote_addr.IPString().c_str(), _remote_addr.port());

    _connected = true;

    retval = S_SUCCESS;
ExitError:
    return retval;
}

int AuthClient::disconnect()
{
    int retcode = E_ERROR;
    int retval  = E_ERROR;

    if (false == _connected)
    {
        printf("not connect!\n");
        goto ExitError;
    }

    struct linger lg;
    lg.l_onoff = 1;
    lg.l_linger = 0;
    retcode = _sock.setsockopt(SOL_SOCKET, SO_LINGER, (char*)&lg, sizeof(lg));
    retcode = _sock.close();
    _connected = false;

    printf("Disconnected from %s:%d.\n", _remote_addr.IPString().c_str(), _remote_addr.port());
ExitOK:
    retval = S_SUCCESS;
ExitError:
    return retval;
}

int AuthClient::keepAlive()
{
    int retcode = E_ERROR;
    int retval  = E_ERROR;

    char buffer[AGIP_MAX_BUFFER_SIZE] = {0};
    int nbytes = AGIP_MAX_BUFFER_SIZE;
    uint32_t res_seq = 0;

    AGIPHandset    handset;
    AGIPHandsetRes handsetRes;

    // judge whether should we send a heartbeat.
    time_t current_time;
    time(&current_time);
    if (current_time > _last_active_time
        && (current_time - _last_active_time) < TIME_KEEP_ALIVE)
    {
        goto ExitOK;    // do nothing.
    }

    // prepare request data.
    handset.setSequenceID( _ERR_SEQ );

#ifdef USE_DECRYPTION_PROTOCOL
    retcode = handset.getEncodedPDU(buffer, AGIP_MAX_BUFFER_SIZE, &nbytes);
#else
    retcode = handset.getDecodedPDU(buffer, AGIP_MAX_BUFFER_SIZE, &nbytes);
#endif
    if (retcode != S_SUCCESS)
    {
        printf("Construct PDU error! %s:%d.\n", __FILE__, __LINE__);
        retval = retcode;
        goto ExitError;
    }

    // send request.
    if ( (retcode = this->sendMsg(buffer, nbytes)) < 0)    // net error.
    {
        printf("Send message error:%d\n", retcode);
        retval = retcode;
        goto ExitError;
    }

    // receive response.
    nbytes = AGIP_MAX_BUFFER_SIZE;
    bzero(buffer, AGIP_MAX_BUFFER_SIZE);
    do 
    {
        if ( (retcode = this->recvMsg(buffer, &nbytes)) < 0)
        {
            printf("Receive message error:%d\n", retcode);
            retval = retcode;
            goto ExitError;
        }
        printf("Received one message.\n");
#ifdef USE_DECRYPTION_PROTOCOL
        retcode = handsetRes.setEncodedPDU(buffer, nbytes, 0);
#else
        retcode = handsetRes.setDecodedPDU(buffer, nbytes, 0);
#endif
        if (retcode != S_SUCCESS)
        {
            printf("Parse PDU error! %s:%d.\n", __FILE__, __LINE__);
            retval = retcode;
            goto ExitError;
        }
        handsetRes.getSequenceID(&res_seq);
    } while (res_seq < _cur_seq);  // skip no-use msg.

    if (res_seq != _cur_seq)
    {
        printf("Sequence not match! %s:%d.\n", __FILE__, __LINE__);
        retval = E_PDU_ERROR;
        goto ExitError;
    }

    printf("Handset Received!\n");
ExitOK:
    retval = S_SUCCESS;
ExitError:
    return retval;
}


int AuthClient::recvMsg(char* buffer, int* nbytes)
{
    int retval  = E_ERROR;
    int nrecv   = 0;
    int msg_len = 0;

    assert(buffer != NULL && nbytes != NULL);

    if (false == _connected)
        goto ExitError;

    // receive msg length.
    nrecv = _sock.recv_into(buffer, AGIP_LENGTH_FIELD_SIZE);
    if (nrecv < 0)  // socket error || timeout
    {
        retval = nrecv;
        goto ExitError;
    }
    else if(nrecv != AGIP_LENGTH_FIELD_SIZE)
    {
        retval = E_PDU_ERROR;
        goto ExitError;
    }

    // get msg length.
    msg_len = htons(*(uint16_t*)buffer);
    if ( msg_len > *nbytes || msg_len < AGIP_MIN_MSG_SIZE )  // msg length illegal.
    {
        retval = E_PDU_ERROR;
        goto ExitError;
    }

    // receive whole msg.
    nrecv = _sock.recv_into(buffer + AGIP_LENGTH_FIELD_SIZE, msg_len - AGIP_LENGTH_FIELD_SIZE);
    if (nrecv < 0)  // socket error || timeout
    {
        retval = nrecv;
        goto ExitError;
    }
    else if (nrecv != msg_len - AGIP_LENGTH_FIELD_SIZE) // msg length illegal.
    {
        retval = E_PDU_ERROR;
        goto ExitError;
    }

    *nbytes = msg_len;
    retval = S_SUCCESS;
ExitError:
    if (retval == E_SYS_NET_CLOSED)
        this->disconnect();

    return retval;
}


int AuthClient::sendMsg(const char* buffer, int nbytes)
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

    int nsend = _sock.send_all(buffer, nbytes);
    if (nsend < 0)
    {
        retval = nsend;
        goto ExitError;
    }
    else if (nsend != nbytes) // unreasonable error.
    {
        retval = -1;
        goto ExitError;
    }

    // record active time.
    _last_active_time = time(0);
    retval = S_SUCCESS;
ExitError:
    if (retval == E_SYS_NET_CLOSED)
        this->disconnect();

    return retval;
}


