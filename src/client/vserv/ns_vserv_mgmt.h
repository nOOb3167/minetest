#ifndef _NS_VSERV_MGMT_H_
#define _NS_VSERV_MGMT_H_

#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <stdexcept>

#include <enet/enet.h>

#include <client/vserv/ns_vserv_clnt_iface.h>
#include <network/address.h>
#include <network/networkpacket.h>
#include <threading/thread.h>

#define GS_IDENTER_REQUEST_INTERVAL_MS 500

class GsMgmtSend
{
public:
	GsMgmtSend(ENetPeer *peer);

	void send(NetworkPacket *packet);

private:
	ENetPeer * m_peer = NULL;
};

class GsIdenter
{
public:
	GsIdenter();

	bool haveHigherGeneration(uint32_t generation);
	bool isWanted();
	void update(long long timestamp, GsMgmtSend *send);
	void mergeIdVec(const std::vector<uint16_t> &id_vec, uint32_t generation);
	const std::map<uint16_t, GsName> & getIdNameMap();

private:
	std::map<uint16_t, GsName> m_id_name_map;
	uint32_t  m_generation_have = 0;
	long long m_timestamp_last_requested = 0;
};

class VServMgmt
{
public:
	class VServMgmtThread : public Thread
	{
	public:
		VServMgmtThread(VServMgmt *mgmt);

		void * run();

	private:
		VServMgmt * m_mgmt = NULL;
	};

	VServMgmt(VServClntCtl *ctl, bool ipv6, uint32_t port, const char *hostname);
	~VServMgmt();

	void waitConnectAndStart();
	void threadFunc();
	void updateOther(long long timestamp, const std::vector<ENetEvent> &evt_vec);
	void packetFill(uint8_t *packetdata, size_t packetsize, NetworkPacket *io_packet);
	void processPacket(long long timestamp, uint8_t *packetdata, size_t packetsize);

private:
	VServClntCtl * m_ctl = NULL;

	std::unique_ptr<GsIdenter> m_identer;

	std::unique_ptr<ENetHost> m_host;
	std::unique_ptr<ENetPeer> m_peer;
	std::unique_ptr<GsMgmtSend> m_send;
	std::unique_ptr<VServMgmtThread> m_thread;
	uint32_t m_thread_exit_code;
	Address  m_addr;
};

int vserv_enet_global_init();

#endif /* _NS_VSERV_MGMT_H_ */
