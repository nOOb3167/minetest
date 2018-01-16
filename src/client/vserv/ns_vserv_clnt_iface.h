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
#include <client/vserv/ns_vserv_hud.h>
#include <client/vserv/ns_vserv_mgmt.h>

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
		m_hud(NULL),
		m_clnt(NULL),
		m_mgmt(NULL),
		m_queue(),
		m_mutex(),
		m_pressed(false),
		m_blk(0)
	{}

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

	static void s_init(uint32_t vserv_port, uint32_t vserv_mgmt_port, const char *vserv_hostname)
	{
		if (g_vserv_clnt_ctl != NULL)
			throw std::logic_error("vserv global already inited");

		VServClntCtl *ctl = new VServClntCtl();
		VServHud  *hud  = new VServHud();
		// FIXME: ipv6 false
		VServClnt *clnt = new VServClnt(ctl, false, vserv_port     , vserv_hostname);
		VServMgmt *mgmt = new VServMgmt(ctl, false, vserv_mgmt_port, vserv_hostname);

		ctl->m_hud  = hud;
		ctl->m_clnt = clnt;
		ctl->m_mgmt = mgmt;

		g_vserv_clnt_ctl = ctl;
	}

	static void s_connect_ident(VServClntCtl *ctl, const char *player_name, const char *serv_name)
	{
		if (!serv_name || strlen(serv_name) == 0)
			serv_name = "serv_name_was_empty_dummy";
		ctl->msgPushName(player_name, serv_name);
	}

	static void s_keyevent(VServClntCtl *ctl, bool is_down)
	{
		/* start setting keys on !down->down transition */
		if (is_down) {
			ctl->m_pressed = true;
		}

		/* stop sending keys (and ready for next block) on down->!down transition */
		if (! is_down && ctl->m_pressed) {
			ctl->m_pressed = false;
			ctl->m_blk += 1;
		}

		if (ctl->m_pressed)
			ctl->m_clnt->setKeys((('s' & 0xFF) << 0) | (ctl->m_blk << 8));
		else
			ctl->m_clnt->setKeys(0);
	}

	static void s_drawhud(VServClntCtl *ctl)
	{
		ctl->m_hud->drawSpectr();
	}

	static void s_enqueueframehud(VServClntCtl *ctl, std::string frame)
	{
		ctl->m_hud->enqueueFrame(std::move(frame));
	}

	static void s_enqueueframehud2(VServClntCtl *ctl, std::string frame)
	{
		ctl->m_hud->enqueueFrame2(std::move(frame));
	}

private:
	VServHud * m_hud  = NULL;
	VServClnt *m_clnt = NULL;
	VServMgmt *m_mgmt = NULL;
	std::deque<VServClntMsg> m_queue;
	std::mutex m_mutex;

	bool     m_pressed = false;
	uint16_t m_blk = 0;
};

#endif /* _VSERV_CLNT_IFACE_H_ */
