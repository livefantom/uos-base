#ifndef _CONNECTOR_H
#define _CONNECTOR_H

#include "thread.h"
#include "mutex.h"
#include "sockwatcher.h"
#include "connection.h"
#include "authmsg.h"
#include <map>



typedef std::map<int, AuthMsg> MsgMap;
typedef std::pair<int, AuthMsg> MsgPair;
typedef MsgMap::iterator MsgIter;

bool try_parse_http_response(std::string res);
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





