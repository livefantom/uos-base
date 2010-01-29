#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "socket.h"


#define CONN_BUF_SZ   4096


struct ConnProp
{
    char	remote_host[MAX_PATH];
    int32_t	remote_port;
    char	http_uri[MAX_PATH];
    int32_t	timeout_secs;
    bool	keep_alive;
};


class Connection : public uos::Socket
{
    friend class Connector;
    enum CONN_STATE
    {
        S_FREE,
        S_CONNECTING,
        S_READING,
        S_WRITING,
        S_DONE
    };
public:
    Connection(const ConnProp& prop);
    ~Connection();

	int doRead();
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
    Connection();
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


#endif//(_CONNECTION_H)





