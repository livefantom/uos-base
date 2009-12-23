#include "authmsg.h"
#include "OperationCode.h"
#include "pfauth.h"
#include <sys/time.h>
#include <cstdlib>


#ifdef E_ERROR
#   undef E_ERROR
#   define E_ERROR      -1
#endif

#define INFOLOG	(PfAuth::singleton()->logger()).info
#define DEBUGLOG	(PfAuth::singleton()->logger()).debug


std::string AuthMsg::encodeRequest() const
{
    std::string req;
    switch ( game_id )
    {
    case 707:	// 4399.com
        switch ( cmd_id )
        {
        case 0x10003801:
            req = "userid="+user_id+"&username="+user_name+"&time="+time+"&flag="+flag;
            break;
        case 0x10003304:
            req = "userName="+user_name+"&password="+time+"&sign="+flag;
            break;
        default:
 	        DEBUGLOG("AuthMsg::encodeRequest | CommandID `%d' is not support!\n", cmd_id);
            break;
        }
        break;
    case 709:	// 91wan.com
        switch ( cmd_id )
        {
        case 0x10003801:
            req = "action=recheck&userName="+user_name+"&time="+time+"&sign="+flag;
            break;
        case 0x10003304:
            req = "userName="+user_name+"&password="+time+"&sign="+flag;
            break;
        default:
 	        DEBUGLOG("AuthMsg::encodeRequest | CommandID `%d' is not support!\n", cmd_id);
            break;
        }
        break;
    default:
        DEBUGLOG("AuthMsg::encodeRequest | GameID `%d' is not support!\n", game_id);
        break;
    }
    return req;
}

const AuthMsg& AuthMsg::decodeResponse(std::string res)
{
    int pos1 = 0;
    if ( res.length() > 0 )
    {
        if ( ( pos1 = res.find("|") ) != -1 )
        {
            retcode = atoi( ( res.substr(0, pos1) ).c_str() );
            adult = atoi( ( res.substr(pos1+1) ).c_str() );
        }
        else
        {
            retcode = atoi( res.c_str() );
            adult = 0;
        }
        // convert operation code.
        switch (retcode)
        {
        case 0:
            retcode = S_SUCCESS;
            break;
        case 1:
            retcode = E_ACCOUNT_NOT_FOUND;
            break;
        case 2:
        case 3:
        case 4:
        case 5:
            retcode = E_JOINT_ACCOUNT_ERROR;
            break;
        default:
            retcode = E_JOINT_MSG_ERROR;
            break;
        }
    }
    else
    {
        retcode = E_JOINT_MSG_ERROR;
        adult = 0;
    }
    if (E_JOINT_MSG_ERROR == retcode)
    {
        INFOLOG( "AuthMsg::decodeResponse | Parsed content is following:\n%s\n", res.c_str() );
    }
    return *this;
}




