#ifndef _CONNECTOR_H
#define _CONNECTOR_H

#include "socket.h"

struct ConnProperty
{
    char    svr_name[MAX_PATH];
    char    cmd_key[MAX_PATH];
    char    content_type[MAX_PATH];
    int     svr_port;
    int     rcv_timeout;    // receive msg timeout, by milliseconds.
    int     hb_interval;    // heartbeat interval, by seconds.
};


class ConnPool
{
	friend class Connector;
public:
	ConnPool(int size, void* pfn)
	{
		// record owner.
		_thr_id = pthread_self();

		_conn_size = size;
		max = _watch.getmaxfiles();
		_conn_size = (max <= size) ? max : size;
		_conn = new Connector[_conn_size];
		
		
		// start to run.
		start();
	};
	
    int asyncSend(char* buffer, int* nbytes, int sequence)
    {
    	if ( _thr_id != pthread_self() )
    		return -1;

    	for( int i=0; i<_size; ++i )
    	{
    		// 线程同步问题？？？
    		if ( true == _conn[i]._free )
    		{
    			_conn[i]._sequence = sequence;
    			_conn[i]._free = false;
    			_conn[i].sendMsg();
    			return 1;
    		}
    	}
    	return -1;
    };
private:
    pthread_t	_thr_id;
	Connector*	_conn;
	int		_conn_size;
	
	SocketWatch	_watch;
	
	void* _pfn;
}

class Connector
{
public:
    Connector(const ConnProperty& proper);
    ~Connector();

    int userAuth(const char* user_name,
        const char* password,
        const char* game_id);

    int recvMsg(char* buffer, int* nbytes);
    int sendMsg(const char* buffer, int nbytes);


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
    uint32_t    _sequence;

	char*	_send_buf;
	char*	_recv_buf;
	bool	_free;

};

#endif//(_CONNECTOR_H)





