#ifndef _AUTHMSG_H
#define _AUTHMSG_H

#include "uosdef.h"
#include <string>

struct AuthMsg
{
	AuthMsg() : gs_ip(0), gs_port(0), seq_id(0), cmd_id(0),
		game_id(0), gateway_id(0), retcode(-1), adult(0), insert_time(0), state(0) {}
	
	std::string encodeRequest() const;
	const AuthMsg& decodeResponse(std::string res);
	
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





#endif//(_AUTHMSG_H)





