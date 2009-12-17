// Third-party platform access Authentication module.
// Copyright(C) 2009 LineKong.com
// @file authclient.h
// @author: Benjamin
// @date: 2009-06-02


#ifndef _AUTH_CLIENT_H
#define _AUTH_CLIENT_H

#include "socket.h"
#include "mutex.h"
#include "thread.h"
#include <map>

struct AuthReq 
{
    uint32_t seq;
    char user_name[32+1];
    char password[32+1];
    char game_id[4+1];
};

struct AuthRes
{
    uint32_t seq;
    char result[12];
};

struct MsgInfo
{
    int status; // 1-sent, 2-received, 3-timeout.
    uint64_t req_msec;  // request time, by milliseconds.
    AuthReq req;
    AuthRes res;
};

typedef std::map<uint32_t, MsgInfo> MsgList;
typedef std::pair<uint32_t, MsgInfo> MsgPair;
typedef MsgList::iterator  MsgIter;


class AuthClient : public uos::Thread
{
public:
    AuthClient();
    virtual ~AuthClient();

    int initialize(const char* ip4, int port, int timeout);
    int release();

    int setRequest(const AuthReq& req);
    int getResponse(AuthRes& res, int sequence);

protected:
    virtual void run();
    virtual int serialize(char* dst, int* dst_len, void* obj);
    virtual int deserialize(void* obj, char* src, int src_len);
    void onRecv();
    void onSend();
    void onError();

    int keepAlive();
    int recvMsg(char* buffer, int* nbytes);
    int sendMsg(const char* buffer, int nbytes);

    int reconnect();
    int disconnect();

private:
    uos::Socket     _sock;
    uos::SockAddr   _remote_addr;
    int         _timeout;
    bool        _connected;
    time_t      _last_active_time;
    uint32_t    _cur_seq;

    uint32_t    _sequence;
    MsgList     _msg_list;
    uos::Mutex    _list_mtx;

};


#endif//(_AUTH_CLIENT_H)



