#ifndef _CONNECTOR_H
#define _CONNECTOR_H

#include "thread.h"
#include "mutex.h"
#include "sockwatcher.h"
#include "connection.h"
#include <map>


struct AuthMsg
{
	AuthMsg() : gs_ip(0), gs_port(0), seq_id(0), cmd_id(0),
		game_id(0), gateway_id(0), retcode(-1), adult(0), insert_time(0), state(0) {}
	uint32_t	gs_ip;
	uint32_t	gs_port;
	uint32_t	seq_id;
	uint32_t	cmd_id;
	uint32_t	game_id;
	uint32_t	gateway_id;
	std::string user_id;
	std::string user_name;
	std::string password;
	std::string time;
	std::string flag;
	int32_t		retcode;
	int32_t		adult;
	// processing control.
	uint64_t	insert_time;
	int32_t		state;
};

typedef std::map<int, AuthMsg> MsgMap;
typedef std::pair<int, AuthMsg> MsgPair;
typedef MsgMap::iterator MsgIter;

std::string ftxy4399_request_encode(const AuthMsg& msg, std::string host, int port, std::string uri);
void ftxy4399_response_decode(std::string res, AuthMsg& msg);


class Connector : public uos::Thread
{
public:
    Connector(uint32_t size, uint32_t queue_max, const ConnProp& prop);
    ~Connector();

    int sendRequest(const AuthMsg& req_msg, uint32_t req_seq);
	int recvResponse(AuthMsg& res_msg, uint32_t* res_seq);

protected:
    virtual void run();

    int getRequest(char* buffer, uint32_t* nbytes, uint32_t* sequence);
    int setResponse(int retcode, const char* buffer, uint32_t nbytes, uint32_t sequence);

private:
    Connection*	_conn;
    ConnProp	_prop;
    uint32_t	_conn_size;
    uint32_t	_conn_cnt;
    SockWatcher	_watch;

    bool		_terminate;

    MsgMap		_msg_map;
    uint32_t	_queue_max;
    uos::Mutex	_mtx;
};



#endif//(_CONNECTOR_H)





