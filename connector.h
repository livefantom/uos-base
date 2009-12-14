#ifndef _CONNECTOR_H
#define _CONNECTOR_H

#include "thread.h"
#include "socket.h"
#include "sockwatcher.h"

struct ConnProperty
{
    char    svr_name[MAX_PATH];
    char    cmd_key[MAX_PATH];
    char    content_type[MAX_PATH];
    int     svr_port;
    int     rcv_timeout;    // receive msg timeout, by milliseconds.
    int     hb_interval;    // heartbeat interval, by seconds.
};



class Connector : public uos::Socket
{
    friend class ConnPool;
    enum CONN_STATE
    {
        S_FREE,
        S_CONNECTING,
        S_READING,
        S_WRITING,
        S_DONE
    };
public:
    Connector(){};
    ~Connector();

    int do_read();
    int do_write();
    int do_connect();

    void setState(uint32_t s)
    {
        _state = s;
        _last_active_time = time(0);
    }

    bool isFree() const
    {
        return ( S_FREE == _state );
    }

    bool isConnecting() const
    {
        return ( S_CONNECTING == _state );
    }

    bool isReading() const
    {
        return ( S_READING == _state );
    }

    bool isWriting() const
    {
        return ( S_WRITING == _state );
    }

    bool isDone() const
    {
        return ( S_DONE == _state );
    }

    bool isTimeout() const
    {
        return ( time(0) > _last_active_time + _timeout );
    }

protected:
    int disconnect();

private:
    time_t      _last_active_time;
    uint32_t	_timeout;
    uint32_t    _sequence;

    char*		_wr_buf;
    char*		_rd_buf;
    uint32_t	_wr_size;
    uint32_t	_rd_size;
    uint32_t	_wr_idx;
    uint32_t	_rd_idx;

    uint32_t	_state;

};

typedef int (*GetReqFunc)(char*, uint32_t*, uint32_t*);
typedef int (*SetResFunc)(int32_t, const char*, uint32_t, uint32_t);

struct ConnProp
{
    std::string remote_host;
    uint32_t	remote_port;
    uint32_t	timeout_secs;
};

class ConnPool : public uos::Thread
{
public:
    ConnPool(uint32_t size, GetReqFunc pfn_req, SetResFunc pfn_res, const ConnProp& prop)
    {
        _conn_size = size;
        _pfn_req = pfn_req;
        _pfn_res = pfn_res;
        uint32_t max = 10;//_watch.getmaxfiles();
        _conn_size = (max <= size) ? max : size;
        _conn = new Connector[_conn_size];

        // start to run.
        start();
    };

    ~ConnPool(){}

    virtual void run();

private:
    Connector*	_conn;
    uint32_t	_conn_size;
    uint32_t	_conn_cnt;

    SockWatcher	_watch;

    GetReqFunc  _pfn_req;
    SetResFunc  _pfn_res;
    bool		_terminate;

    ConnProp	_prop;
};

#endif//(_CONNECTOR_H)





