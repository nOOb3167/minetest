#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <stdexcept>

#include <enet/enet.h>

#include <client/vserv/ns_vserv_clnt_iface.h>
#include <client/vserv/ns_vserv_mgmt.h>
#include <debug.h>
#include <network/address.h>
#include <network/networkpacket.h>
#include <porting.h>
#include <threading/thread.h>

GsMgmtSend::GsMgmtSend(ENetPeer *peer):
	m_peer(peer)
{}

void GsMgmtSend::send(NetworkPacket *packet)
{
	ENetPacket *pkt = NULL;

	if (!(pkt = enet_packet_create(packet->getU8Ptr(0), packet->getSize(), ENET_PACKET_FLAG_RELIABLE)))
		throw std::runtime_error("VServ mgmt packet create");

	/* enet_peer_send takes ownership of packet on success */

	if (0 > enet_peer_send(m_peer, 0, pkt)) {
		enet_packet_destroy(pkt);
		throw std::runtime_error("VServ mgmt packet send");
	}
}

GsIdenter::GsIdenter() = default;

bool GsIdenter::haveHigherGeneration(uint32_t generation)
{
	return m_generation_have > generation;
}

bool GsIdenter::isWanted()
{
	return true;
}

void GsIdenter::update(long long timestamp, GsMgmtSend *send)
{
	/* no update needed at all */

	if (! isWanted())
		return;

	/* update work needed - but not yet */

	if (timestamp < m_timestamp_last_requested + GS_IDENTER_REQUEST_INTERVAL_MS)
		return;

	/* update work - send / resent the ident message */

	NetworkPacket packet;

	packet << (uint8_t) GS_VSERV_CMD_IDGET << (uint32_t) m_generation_have;

	send->send(&packet);
}

void GsIdenter::mergeIdVec(const std::vector<uint16_t> &id_vec, uint32_t generation)
{
	std::map<uint16_t, GsName> id_name_map;

	if (m_generation_have > generation) // should not happen
		throw std::runtime_error("VServ mgmt identer mergeIdVec generation");

	/* new map includes everything from IdVec */

	for (size_t i = 0; i < id_vec.size(); i++) {
		GsName tmp;
		tmp.m_name; /*dummy*/
		tmp.m_serv; /*dummy*/
		tmp.m_id = id_vec[i];
	}

	if (id_name_map.size() != id_vec.size())
		throw std::runtime_error("VServ mgmt identer mergeIdVec unique");

	/* new map also merges from existing */

	for (auto it = m_id_name_map.begin(); it != m_id_name_map.end(); ++it) {
		auto it2 = id_name_map.find(it->first);
		if (it2 == id_name_map.end())
			continue;
		it2->second.m_name = it->second.m_name;
		it2->second.m_serv = it->second.m_serv;
	}

	m_id_name_map = std::move(id_name_map);
	m_generation_have = generation;
}

const std::map<uint16_t, GsName> & GsIdenter::getIdNameMap()
{
	return m_id_name_map;
}

VServMgmt::VServMgmtThread::VServMgmtThread(VServMgmt *mgmt):
	Thread("vserv_mgmt"),
	m_mgmt(mgmt)
{}

void * VServMgmt::VServMgmtThread::run()
{
	BEGIN_DEBUG_EXCEPTION_HANDLER

	m_mgmt->threadFunc();

	END_DEBUG_EXCEPTION_HANDLER

	this->stop();

	return nullptr;
}

VServMgmt::VServMgmt(VServClntCtl *ctl, bool ipv6, uint32_t port, const char *hostname):
	m_identer(new GsIdenter()),
	m_host(),
	m_peer(),
	m_send(),
	m_thread(new VServMgmtThread(this)),
	m_thread_exit_code(0),
	m_addr(0, 0, 0, 0, port)
{
	assert(!ipv6);

	ENetAddress enet_connect_addr = {};

	if (0 > enet_address_set_host(&enet_connect_addr, hostname))
		throw std::runtime_error("VServ mgmt address set host");
	enet_connect_addr.port = port;

	m_host.reset(enet_host_create(NULL, 128, 1, 0, 0));
	if (! m_host)
		throw std::runtime_error("VServ mgmt host create");
		
	m_peer.reset(enet_host_connect(m_host.get(), &enet_connect_addr, 1, 0));
	if (! m_peer)
		throw std::runtime_error("VServ mgmt host connect");

	m_send.reset(new GsMgmtSend(m_peer.get()));

	// FIXME: waiting in constructor
	waitConnectAndStart();
}

VServMgmt::~VServMgmt()
{
	/* order of release matters */
	// FIXME: create unique_ptrs with custom destructors
	m_peer.reset();
	m_host.reset();
}

void VServMgmt::waitConnectAndStart()
{
	ENetEvent evt = {};

	while (true) {
		int r = enet_host_service(m_host.get(), &evt, 100);
		if (r < 0)
			throw std::runtime_error("VServ mgmt connect");
		if (r == 0)
			continue;
		assert(evt.type == ENET_EVENT_TYPE_CONNECT);
		assert(evt.peer == m_peer.get());
		assert(evt.data == NULL);
		break;
	}

	m_thread->start();
}

void VServMgmt::threadFunc()
{
	long long timestamp_last_run = porting::getTimeMs();

	while (! m_thread->stopRequested()) {
		long long timestamp_before_wait = porting::getTimeMs();
		if (timestamp_before_wait < timestamp_last_run) /* backwards clock? wtf? */
			timestamp_before_wait = LLONG_MAX;          /* just ensure processing runs immediately */
		std::vector<ENetEvent> evt_vec;
		ENetEvent evt = {};
		int r = 0;
		/* perform first ENet event check using enet_host_service */
		if (0 > (r = enet_host_service(m_host.get(), &evt, GS_CLNT_ONE_TICK_MS)))
			throw std::runtime_error("VServ mgmt host service");
		if (0 < r) {
			evt_vec.push_back(evt);
			/* perform ENet event checks until none pending using enet_host_check_events */
			while (true) {
				if (0 > (r = enet_host_check_events(m_host.get(), &evt)))
					throw std::runtime_error("VServ mgmt host check_events");
				if (0 < r)
					evt_vec.push_back(evt);
			}
		}
		timestamp_last_run = porting::getTimeMs();
		updateOther(timestamp_last_run, evt_vec);
	}
}

void VServMgmt::updateOther(long long timestamp, const std::vector<ENetEvent> &evt_vec)
{
	/* network receiving and general network processing */

	m_identer->update(timestamp, m_send.get());

	for (size_t i = 0; i < evt_vec.size(); i++) {
		// FIXME: handle other types, especially ENET_EVENT_TYPE_DISCONNECT
		assert(evt_vec[i].type == ENET_EVENT_TYPE_RECEIVE);
		processPacket(timestamp, evt_vec[i].packet->data, evt_vec[i].packet->dataLength);
	}
}

void VServMgmt::packetFill(uint8_t *packetdata, size_t packetsize, NetworkPacket *io_packet)
{
	// FIXME: inefficient
	std::unique_ptr<uint8_t[]> pp(new uint8_t[2 + packetsize]);
	pp[0] = 0; pp[1] = 0;
	memcpy(pp.get() + 2, packetdata, packetsize);
	io_packet->putRawPacket(pp.get(), 2 + packetsize, -1);
}

void VServMgmt::processPacket(long long timestamp, uint8_t *packetdata, size_t packetsize)
{
	NetworkPacket packet;

	packetFill(packetdata, packetsize, &packet);

	uint8_t cmd = 0;

	packet >> cmd;

	switch (cmd) {
	case GS_VSERV_CMD_IDS:
	{
		uint32_t generation = 0;
		uint32_t id_num = 0;

		std::vector<uint16_t> id_vec;

		packet >> generation >> id_num;

		// FIXME: unbounded iteration
		for (size_t i = 0; i < id_num; i++) {
			uint16_t id = 0;
			packet >> id;
			id_vec.push_back(id);
		}

		/* reliability-codepath .. but ENet reliable packets should have sufficed */

		if (! m_identer->isWanted())
			return;
		if (m_identer->haveHigherGeneration(generation))
			return;

		/* seems legit, apply */

		m_identer->mergeIdVec(id_vec, generation);

		/* reply with how the ids need to be grouped */

		NetworkPacket outPacket;

		const std::map<uint16_t, GsName> &id_name_map = m_identer->getIdNameMap();

		outPacket << (uint8_t) GS_VSERV_M_CMD_GROUPSET << (uint32_t) id_name_map.size();

		for (auto it = id_name_map.begin(); it != id_name_map.end(); ++it)
			outPacket << it->first;

		outPacket << (uint16_t) id_name_map.size();

		m_send->send(&outPacket);
	}
	break;

	default:
		assert(0);
	}
}

int vserv_enet_global_init()
{
	return !! enet_initialize();
}
