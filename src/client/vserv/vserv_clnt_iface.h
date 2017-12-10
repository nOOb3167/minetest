#ifndef _VSERV_CLNT_IFACE_H_
#define _VSERV_CLNT_IFACE_H_

#include <string>
#include <mutex>
#include <deque>

#include <client/vserv/vserv_misc.h>
#include <client/vserv/vserv_clnt_helpers.h>
#include <threading/mutex_auto_lock.h>

struct GsVServClnt;  // forward decl only

enum VServClntMsgType
{
	VSERV_CLNT_MSG_TYPE_NONE = 0,
	VSERV_CLNT_MSG_TYPE_NAME = 1,
};

struct VServClntMsg
{
	enum VServClntMsgType mType;
	struct GsName mName; /* VSERV_CLNT_MSG_TYPE_NAME */
};

class VServClntCtl
{
public:
	VServClntCtl() :
		mQueue(),
		mMutex()
	{}

	void ClntSet(struct GsVServClnt *Clnt)
	{
		mClnt = Clnt;
	}

	void MsgPush(const VServClntMsg &Msg)
	{
		MutexAutoLock Lock(mMutex);
		mQueue.push_back(Msg);
	}

	void MsgPushName(const std::string &Name, const std::string &Serv)
	{
		VServClntMsg Msg;
		Msg.mType = VSERV_CLNT_MSG_TYPE_NAME;
		Msg.mName.mName = Name;
		Msg.mName.mServ = Serv;
		MsgPush(Msg);
	}

	VServClntMsg MsgPop()
	{
		MutexAutoLock Lock(mMutex);
		GS_ASSERT(! mQueue.empty());
		VServClntMsg Front = mQueue.front();
		mQueue.pop_front();
		return std::move(Front);
	}

	bool MsgHas()
	{
		MutexAutoLock Lock(mMutex);
		return ! mQueue.empty();
	}

private:
	struct GsVServClnt *mClnt;
	std::deque<VServClntMsg> mQueue;
	std::mutex mMutex;

};

int gs_vserv_clnt_init(uint32_t VServPort, const char *VServHostNameBuf);
int gs_vserv_clnt_setup(uint32_t VServPort, const char *VServHostNameBuf, struct GsVServClnt **oClnt);
int gs_vserv_clnt_connect_ident(VServClntCtl *Ctl, const char *PlayerName, const char *ServName);

/* global variable */
// FIXME: defined in vserv_clnt_test.cpp for now
extern VServClntCtl *g_vserv_clnt_ctl;

#endif /* _VSERV_CLNT_IFACE_H_ */
