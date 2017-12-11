#include <cstdlib>
#include <climits>

#include <atomic>
#include <thread>
#include <string>
#include <deque>
#include <random>
#include <utility>

#include <client/vserv/vserv_clnt.h>
#include <client/vserv/vserv_clnt_iface.h>
#include <client/vserv/vserv_helpers.h>
#include <client/vserv/vserv_log.h>
#include <client/vserv/vserv_misc.h>
#include <debug.h>
#include <network/socket.h>
#include <porting.h>
#include <threading/thread.h>

#define GS_CLNT_ONE_TICK_MS 20

// FIXME: defined here for now
VServClntCtl *g_vserv_clnt_ctl = NULL;

class VServThread : public Thread
{
public:

	VServThread(struct GsVServClnt *Clnt):
		Thread("vserv_clnt"),
		mClnt(Clnt)
	{}

	void *run();

private:
	struct GsVServClnt *mClnt;
};

struct GsVServClnt
{
	struct GsVServClntCtx *mCtx;
	sp<UDPSocket> mSocket;
	sp<VServThread> mThread;
	uint32_t mThreadExitCode;
	struct GsVServClntAddress mAddr;
	std::atomic<uint32_t> mKeys;
	std::mt19937                            mRandGen;
	std::uniform_int_distribution<uint32_t> mRandDis;
};

void threadfunc(VServThread *Thread, struct GsVServClnt *Clnt);

void threadfunc(VServThread *Thread, struct GsVServClnt *Clnt)
{
	int r = 0;

	log_guard_t Log(GS_LOG_GET("vserv_clnt"));

	// FIXME: temporary testing dummy
	uint16_t BlkDummy = 0;
	long long BlkTimeStampDummy = porting::getTimeMs();

	{
		while (!g_vserv_clnt_ctl->MsgHas())
			sleep_ms(100);
		VServClntMsg Msg = g_vserv_clnt_ctl->MsgPop();
		GS_ASSERT(Msg.mType == VSERV_CLNT_MSG_TYPE_NAME);
		if (!!(r = gs_vserv_clnt_callback_ident(Clnt, Msg.mName.mName.c_str(), Msg.mName.mName.size(), Msg.mName.mServ.c_str(), Msg.mName.mServ.size(), porting::getTimeMs())))
			GS_GOTO_CLEAN();
	}

	long long TimeStampLastRun = porting::getTimeMs();

	while (! Thread->stopRequested()) {
		uint32_t Keys = 0;
		long long TimeStampBeforeWait = porting::getTimeMs();
		bool WaitIndicatesDataArrived = 0;
		if (TimeStampBeforeWait < TimeStampLastRun) /* backwards clock? wtf? */
			TimeStampBeforeWait = LLONG_MAX;        /* just ensure processing runs immediately */
		long long TimeRemainingToFullTick = GS_CLNT_ONE_TICK_MS - GS_MIN(TimeStampBeforeWait - TimeStampLastRun, GS_CLNT_ONE_TICK_MS);
		WaitIndicatesDataArrived = Clnt->mSocket->WaitData(TimeRemainingToFullTick); /* note indication is not actually used */
		TimeStampLastRun = porting::getTimeMs();
		Keys = Clnt->mKeys.load();
		// FIXME: temporary testing dummy
		BlkDummy = (TimeStampLastRun - BlkTimeStampDummy) / 3000; /* increment new block every 3s */
		Keys = ('s' << 0) | (BlkDummy << 8);
		if (!!(r = gs_vserv_clnt_callback_update_other(Clnt, TimeStampLastRun, Keys)))
			GS_GOTO_CLEAN();
	}

clean:
	Clnt->mThreadExitCode = r;
}

void * VServThread::run()
{
	int r = 0;

	BEGIN_DEBUG_EXCEPTION_HANDLER_();

	threadfunc(this, mClnt)

	END_DEBUG_EXCEPTION_HANDLER_();

	this->stop();

	return nullptr;
}

int gs_vserv_clnt_ctx_set(struct GsVServClnt *Clnt, struct GsVServClntCtx *Ctx)
{
	GS_ASSERT(! Clnt->mCtx);
	Clnt->mCtx = Ctx;
	return 0;
}

int gs_vserv_clnt_ctx_get(struct GsVServClnt *Clnt, struct GsVServClntCtx **oCtx)
{
	*oCtx = Clnt->mCtx;
	return 0;
}

int gs_vserv_clnt_receive(
	struct GsVServClnt *Clnt,
	struct GsVServClntAddress *ioAddrFrom,
	uint8_t *ioDataBuf, size_t DataSize, size_t *oLenData)
{
	int r = 0;

	Address MtAddrFrom(-1, -1);

	int LenData = 0;

	/* -1 return code aliasing error and lack-of-progress conditions.. nice api minetest */
	if (-1 == (LenData = Clnt->mSocket->Receive(MtAddrFrom, ioDataBuf, DataSize))) {
		/* pretend -1 just means EAGAIN/EWOULDBLOCK/notreadable */
		LenData = 0;
		GS_ERR_NO_CLEAN(0);
	}

noclean:
	if (ioAddrFrom) {
		ioAddrFrom->mSinAddr = MtAddrFrom.getFamily();
		ioAddrFrom->mSinPort = MtAddrFrom.getPort();
		ioAddrFrom->mSinAddr = ntohl(MtAddrFrom.getAddress().sin_addr.s_addr);
	}
	if (oLenData)
		*oLenData = LenData;

clean:

	return r;
}

int gs_vserv_clnt_send(struct GsVServClnt *Clnt, const uint8_t *DataBuf, size_t LenData)
{
	int r = 0;

	Address MtDest(Clnt->mAddr.mSinAddr, Clnt->mAddr.mSinPort);

	Clnt->mSocket->Send(MtDest, DataBuf, LenData);

clean:

	return r;
}

int gs_vserv_clnt_random_uint(struct GsVServClnt *Clnt, uint32_t *oRand)
{
	*oRand = Clnt->mRandDis(Clnt->mRandGen);
	return 0;
}

int gs_vserv_clnt_setkeys(struct GsVServClnt *Clnt, uint32_t Keys)
{
	Clnt->mKeys.store(Keys);
	return 0;
}

int gs_vserv_clnt_init(uint32_t VServPort, const char *VServHostNameBuf)
{
	int r = 0;

	struct GsVServClnt *Clnt = NULL;

	if (g_vserv_clnt_ctl != NULL)
		GS_ERR_CLEAN(1);

	g_vserv_clnt_ctl = new VServClntCtl();

	if (!!(r = gs_vserv_clnt_setup(VServPort, VServHostNameBuf, &Clnt)))
		GS_GOTO_CLEAN();

	g_vserv_clnt_ctl->ClntSet(GS_ARGOWN(&Clnt));

clean:

	return r;
}

int gs_vserv_clnt_setup(uint32_t VServPort, const char *VServHostNameBuf, struct GsVServClnt **oClnt)
{
	int r = 0;

	struct GsVServClnt *Clnt = NULL;

	std::random_device RandDev;

	struct GsVServClntAddress Addr = {};
	Address MtAddrAny((irr::u32)INADDR_ANY, 0);

	Addr.mSinFamily = AF_INET;
	Addr.mSinPort = VServPort;
	Addr.mSinAddr = 0;

	// FIXME: switch to network/address.h for this functionality
	if (!!(r = UDPSocket::GetHostByName(VServHostNameBuf, &Addr.mSinAddr)))
		GS_GOTO_CLEAN();

	Clnt = new GsVServClnt();
	Clnt->mCtx = NULL;
	Clnt->mSocket = sp<UDPSocket>(new UDPSocket(false));
	Clnt->mThread; /*dummy*/
	Clnt->mThreadExitCode = 0;
	Clnt->mAddr = Addr;
	Clnt->mKeys.store(0);
	Clnt->mRandGen = std::mt19937(RandDev());
	Clnt->mRandDis = std::uniform_int_distribution<uint32_t>();

	if (!!(r = gs_vserv_clnt_callback_create(Clnt)))
		GS_GOTO_CLEAN();

	Clnt->mSocket->Bind(MtAddrAny);

	Clnt->mThread = sp<VServThread>(new VServThread(Clnt));

	Clnt->mThread->start();

	//Clnt->mThread->wait();

	//if (!!(r = Clnt->mThreadExitCode))
	//	GS_GOTO_CLEAN();

	if (oClnt)
		*oClnt = GS_ARGOWN(&Clnt);

clean:

	return r;
}

int gs_vserv_clnt_connect_ident(VServClntCtl *Ctl, const char *PlayerName, const char *ServName)
{
	Ctl->MsgPushName(PlayerName, ServName);
	return 0;
}
