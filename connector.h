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
public:
    Connector(const ConnProperty& proper);
    ~Connector();

    int userAuth(const char* user_name,
        const char* password,
        const char* game_id);

    int recvMsg(char* buffer, int* nbytes);
    int sendMsg(const char* buffer, int nbytes);
    
    int do_read()
    {
    }
    int do_write()
    {
    }
    
    void setFlag(int flag)
    {
		_flag = flag;
		_last_active_time = time(0);
    }
    
    bool timeout()
    {
    	if ( time(0) > _last_active_time + _timeout )
    		return true;
    	else
    		return false;
    }

protected:
    int next_seq();
    int reconnect();
    int disconnect();

private:
    ConnProperty    _proper;
    uos::SockAddr   _remote_addr;
    uos::Socket     _sock;

    bool        _connected;
    time_t      _last_active_time;
    int		_timeout;
    uint32_t    _sequence;

	char*	_wr_buf;
	char*	_rd_buf;
	int		_wr_idx;
	int		_rd_idx;

	int		_buf_size;

	bool	_flag;

};

typedef void (*GetReqFunc)(char*, int*, int*);
typedef void (*SetResFunc)(int, const char*, int, int);

class ConnPool : public uos::Thread
{
	friend class Connector;
	enum CONN_FLAG{
			F_FREE,
			F_CONNECTING,
			F_READING,
			F_WRITING,
			F_DONE
	};
public:
	ConnPool(int size, GetReqFunc pfn_req, SetResFunc pfn_res, const ConnProperty& proper)
	{
		_conn_size = size;
		_pfn_req = pfn_req;
		_pfn_res = pfn_res;
		int max = 10;//_watch.getmaxfiles();
		_conn_size = (max <= size) ? max : size;
		_conn = new Connector[_conn_size];
		
		// start to run.
		start();
	};
	
	~ConnPool(){}

    virtual void run();
    
private:
	Connector*	_conn;
	int		_conn_size;
	int		_conn_cnt;
	
	SockWatcher	_watch;
	
	GetReqFunc _pfn_req;
	SetResFunc _pfn_res;
};

#endif//(_CONNECTOR_H)





