#ifndef _CONNECTOR_H
#define _CONNECTOR_H

#include "thread.h"
#include "mutex.h"
#include "sockwatcher.h"
#include "connection.h"
#include <map>


struct ConnProperty
{
    char    svr_name[MAX_PATH];
    char    cmd_key[MAX_PATH];
    char    content_type[MAX_PATH];
    int     svr_port;
    int     rcv_timeout;    // receive msg timeout, by milliseconds.
    int     hb_interval;    // heartbeat interval, by seconds.
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

class Connector : public uos::Thread
{
public:
    Connector(uint32_t size, const ConnProp& prop)
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

    ~Connector(){}

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





