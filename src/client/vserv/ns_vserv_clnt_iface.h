#ifndef _VSERV_CLNT_IFACE_H_
#define _VSERV_CLNT_IFACE_H_

#include <cstdint>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

#include <threading/mutex_auto_lock.h>

#include <client/vserv/ns_vserv_clnt.h>

class VServClntCtl;
/* global variable */
extern VServClntCtl *g_vserv_clnt_ctl;

enum GsVServGroupMode
{
	GS_VSERV_GROUP_MODE_NONE = 0,
	GS_VSERV_GROUP_MODE_S = 's',
};

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

enum VServClntMsgType
{
	VSERV_CLNT_MSG_TYPE_NONE = 0,
	VSERV_CLNT_MSG_TYPE_NAME = 1,
};

class VServClntMsg
{
public:
	class Name
	{
	public:
		std::string m_name;
		std::string m_serv;
	};

	VServClntMsgType m_type;

	Name m_name; /* VSERV_CLNT_MSG_TYPE_NAME */
};

class VServClntCtl
{
public:
	VServClntCtl() :
		m_queue(),
		m_mutex()
	{}

	void clntSet(VServClnt *clnt)
	{
		m_clnt = clnt;
	}

	void msgPush(const VServClntMsg &msg)
	{
		MutexAutoLock lock(m_mutex);
		m_queue.push_back(msg);
	}

	void msgPushName(const std::string &name, const std::string &serv)
	{
		VServClntMsg msg;
		msg.m_type = (VServClntMsgType) VSERV_CLNT_MSG_TYPE_NAME;
		msg.m_name.m_name = name;
		msg.m_name.m_serv = serv;
		msgPush(msg);
	}

	VServClntMsg msgPop()
	{
		MutexAutoLock lock(m_mutex);
		assert(! m_queue.empty());
		VServClntMsg front = m_queue.front();
		m_queue.pop_front();
		return std::move(front);
	}

	bool msgHas()
	{
		MutexAutoLock Lock(m_mutex);
		return ! m_queue.empty();
	}

	static void s_init(uint32_t vserv_port, const char *vserv_hostname)
	{
		if (g_vserv_clnt_ctl != NULL)
			throw std::logic_error("vserv global already inited");

		g_vserv_clnt_ctl = new VServClntCtl();
		// FIXME: ipv6 false
		VServClnt *clnt = new VServClnt(g_vserv_clnt_ctl, false, vserv_port, vserv_hostname);
		g_vserv_clnt_ctl->clntSet(clnt);
	}

	static void s_connect_ident(VServClntCtl *ctl, const char *player_name, const char *serv_name)
	{
		ctl->msgPushName(player_name, serv_name);
	}

private:
	VServClnt *m_clnt;
	std::deque<VServClntMsg> m_queue;
	std::mutex m_mutex;

};

#endif /* _VSERV_CLNT_IFACE_H_ */
