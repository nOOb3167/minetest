#ifndef _VSERV_CLNT_HELPERS_H_
#define _VSERV_CLNT_HELPERS_H_

#include <stdint.h>

#include <string>

#define GS_CLNT_ARBITRARY_CONNECT_TIMEOUT_MS 5000

#define GS_CLNT_ARBITRARY_USER_TIMEOUT_CHECK_MS 1000  /* check every 1s */
#define GS_CLNT_ARBITRARY_USER_TIMEOUT_MS       5000  /* timing out users showing no network activity for 5s */

#define GS_CLNT_ARBITRARY_PACKET_MAX 4096 /* but mind IP layer fragmentation issues of UDP */

#define GS_VSERV_USER_ID_INVALID 0xFFFF
#define GS_VSERV_USER_ID_SERVFILL 0xFFFF

typedef uint8_t gs_vserv_group_mode_t;
typedef uint16_t gs_vserv_user_id_t;

enum GsVServCmd {
	GS_VSERV_CMD_BROADCAST = 'b',
	GS_VSERV_M_CMD_GROUPSET = 's',
	GS_VSERV_CMD_GROUP_MODE_MSG = 'm',
	GS_VSERV_CMD_IDENT = 'i',
	GS_VSERV_CMD_IDENT_ACK = 'I',
	GS_VSERV_CMD_NAMEGET = 'n',
	GS_VSERV_CMD_NAMES = 'N',
	GS_VSERV_CMD_IDGET = 'd',
	GS_VSERV_CMD_IDS   = 'D',
	GS_VSERV_CMD_PING  = 'p',
};

#define GS_MACRO_VSERV_CMD_ONE(NAME) { NAME, # NAME }
#define GS_MACRO_VSERV_CMD_LIST() { \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_CMD_BROADCAST), \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_M_CMD_GROUPSET), \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_CMD_GROUP_MODE_MSG), \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_CMD_IDENT), \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_CMD_IDENT_ACK), \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_CMD_NAMEGET), \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_CMD_NAMES), \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_CMD_IDGET), \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_CMD_IDS), \
		GS_MACRO_VSERV_CMD_ONE(GS_VSERV_CMD_PING), \
	}
#define GS_MACRO_VSERV_CMD_LIST_VAR(VARNAME) \
	struct { int mNum; const char *mStr; } VARNAME[] = GS_MACRO_VSERV_CMD_LIST(); \
	size_t VARNAME ## Num = sizeof VARNAME / sizeof *VARNAME;

enum GsVServGroupMode
{
	GS_VSERV_GROUP_MODE_NONE = 0,
	GS_VSERV_GROUP_MODE_S = 's',
};

struct GsName
{
	std::string mName;
	std::string mServ;
	uint16_t mId;
};

#endif /* _VSERV_CLNT_HELPERS_H_ */
