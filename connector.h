#ifndef _CONNECTOR_H
#define _CONNECTOR_H

#include "thread.h"
#include "socket.h"
#include "sockwatcher.h"
#include "mutex.h"
#include <map>


#define CONN_BUF_SZ   4096

struct ConnProperty
{
    char    svr_name[MAX_PATH];
    char    cmd_key[MAX_PATH];
    char    content_type[MAX_PATH];
    int     svr_port;
    int     rcv_timeout;    // receive msg timeout, by milliseconds.
    int     hb_interval;    // heartbeat interval, by seconds.
};

struct ConnProp
{
    std::string remote_host;
    uint32_t	remote_port;
    uint32_t	timeout_secs;
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
    Connector(const ConnProp& prop);
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
        return ( time(0) > _last_active_time + _prop.timeout_secs );
    }

protected:
    Connector();
	void setProp(const ConnProp& prop)
	{
		_prop = prop;
	}

    int disconnect();

private:
	ConnProp	_prop;

    time_t      _last_active_time;
    uint32_t    _sequence;

    char		_wr_buf[CONN_BUF_SZ];
    char		_rd_buf[CONN_BUF_SZ];
    uint32_t	_wr_size;
    uint32_t	_rd_size;
    uint32_t	_wr_idx;
    uint32_t	_rd_idx;

    uint32_t	_state;

};


struct AuthMsg
{
	std::string user_id;
	std::string user_name;
	std::string time;
	std::string flag;
	int retcode;
	int adult;
	// process control.
	time_t insert_time;
	int state;
};


typedef std::map<int, AuthMsg> MsgMap;
typedef std::pair<int, AuthMsg> MsgPair;
typedef MsgMap::iterator MsgIter;

std::string ftxy4399_request_encode(const AuthMsg& msg);
void ftxy4399_response_decode(std::string res, AuthMsg& msg);


struct PfAuthCfg
{
    char    log_path[MAX_PATH];
    int     log_level;
    int     log_file_sz;

    ConnProperty    conn_proper;
};



class ConnPool : public uos::Thread
{
public:
    ConnPool(uint32_t size, const ConnProp& prop)
    {
        _conn_size = size;
        uint32_t max = 10;//_watch.getmaxfiles();
        _conn_size = (max <= size) ? max : size;
        _conn = new Connector[_conn_size];
        for (uint32_t i=0; i< _conn_size; ++i)
        {
        	_conn[i].setProp(prop);
        }

        // start to run.
        start();
    };

    ~ConnPool(){}

    int sendRequest(const AuthMsg& req_msg, uint32_t req_seq);
	int recvResponse(AuthMsg& res_msg, uint32_t* res_seq);

protected:
    virtual void run();

    int getRequest(char* buffer, uint32_t nbytes, uint32_t* sequence);
    int setResponse(int retcode, const char* buffer, uint32_t nbytes, uint32_t sequence);

private:
    Connector*	_conn;
    uint32_t	_conn_size;
    uint32_t	_conn_cnt;

    SockWatcher	_watch;

    bool		_terminate;

    ConnProp	_prop;
    MsgMap		_msg_map;
    uos::Mutex	_mtx;
};

#endif//(_CONNECTOR_H)





