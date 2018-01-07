#ifndef _VSERV_CLNT_H_
#define _VSERV_CLNT_H_

#include <cstdint>

#include <atomic>
#include <map>
#include <memory>
#include <random>
#include <string>

#include <client/vserv/ns_vserv_openal_include.h>
#include <network/address.h>
#include <network/networkpacket.h>
#include <network/socket.h>
#include <threading/thread.h>

#define GS_VSERV_USER_ID_INVALID  0xFFFF
#define GS_VSERV_USER_ID_SERVFILL 0xFFFF

#define GS_CLNT_ONE_TICK_MS 20
#define GS_CLNT_ARBITRARY_IDENT_RESEND_TIMEOUT 100

#define GS_48KHZ 48000

#define GS_PINGER_REQUEST_INTERVAL_MS 1000
#define GS_RECORD_ARBITRARY_BUFFER_SAMPLES_NUM 48000
#define GS_PLAYBACK_FLOW_DELAY_EXPIRY_MS (GS_OPUS_FRAME_DURATION_20MS * 3 * 2)  // FIXME: adjust expiry
#define GS_PLAYBACK_FLOW_LEADING_SEQUENCE_LOSS_THRESHOLD (50 * 3) /* at 20ms there are 50 frames per second. (50*3) designating 3s of continued loss */
#define GS_PLAYBACK_FLOW_DELAY_MS 50

#define GS_OPUS_FRAME_DURATION_20MS 20
#define GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM ((GS_48KHZ / 1000) /*samples/msec*/ * GS_OPUS_FRAME_DURATION_20MS /*20ms (one Opus frame)*/)

class VServClntCtl;

class GsSend
{
public:
	GsSend(UDPSocket *sock, Address addr):
		m_sock(sock),
		m_addr(addr)
	{}

	void send(NetworkPacket *packet)
	{
		m_sock->Send(m_addr, packet->getU8Ptr(0), packet->getSize());
	}

private:
	UDPSocket *m_sock;
	Address    m_addr;
};

class GsName
{
public:
	GsName() = default;

	std::string m_name;
	std::string m_serv;
	uint16_t m_id = GS_VSERV_USER_ID_SERVFILL;
};

class GsRenamer
{
public:
	GsRenamer() = default;
	GsRenamer(const std::string &name_want, const std::string &serv_want, long long timestamp, uint32_t rand);

	bool matchingRand(uint32_t rand);
	bool isWanted();
	void identEmit(NetworkPacket *packet);
	void update(GsSend *send, long long timestamp);
	void reset();
	GsName wantedName(uint16_t id);

private:
	std::string m_name_want;
	std::string m_serv_want;
	long long m_timestamp_last_requested = 0;
	uint32_t m_rand_last_requested = 0;
};

class GsPinger
{
public:
	GsPinger() = default;

	void update(GsSend *send, long long timestamp);

private:
	long long m_timestamp_last_ping = 0;
};

class GsRecord
{
public:
	typedef ::std::unique_ptr<ALCdevice, void(*)(ALCdevice *device)> unique_ptr_alcdevice;

	GsRecord();

	void start();
	void stop();
	void captureDrain(size_t SampSize, size_t FraNumSamp, std::string *FraBuf);

	static void deleteDevice(ALCdevice *device);

private:
	unique_ptr_alcdevice m_cap_device;
};

class GsPlayBack
{
public:
	typedef ::std::unique_ptr<ALuint, void (*)(ALuint *buffer)> unique_ptr_aluint;

	struct PBFlowKey
	{
		uint16_t m_id;
		uint16_t m_blk;
	};

	struct pb_flow_key_less_t
	{
		bool operator()(const PBFlowKey &a, const PBFlowKey &b) const
		{
			return a.m_id != b.m_id ? (a.m_id < b.m_id) : (a.m_blk < b.m_blk);
		}
	};

	struct PBFlow
	{
		PBFlow(unique_ptr_aluint source, long long timestamp_first_receipt):
				m_source(std::move(source)),
				m_timestamp_first_receipt(timestamp_first_receipt),
				m_next_seq(0)
		{}

		unique_ptr_aluint m_source;
		/* Seq -> Buf */
		std::map<uint16_t, std::string> m_map_buf;
		long long m_timestamp_first_receipt = 0;
		size_t m_next_seq = 0;
	};

	struct PBId
	{
		uint16_t m_blk_floor = 0;
	};

	GsPlayBack();

	void packetInsert(long long timestamp, uint16_t id, uint16_t blk, uint16_t seq,
		const char *data, size_t len_data);
	void harvest(long long timestamp, std::vector<std::pair<ALuint, unique_ptr_aluint> > *out_buffer_vec);
	void harvestAndEnqueue(long long timestamp);
	void dequeue();
	static void dequeueOne(ALuint source);
	void ensurePlaying();
	static void ensureStoppedOne(ALuint source);
	void expireFlows(long long timestamp);
	bool isLiveFlow(const PBFlowKey &key, long long timestamp);

	static void deleteBuffer(ALuint *buffer);
	static void deleteSource(ALuint *source);

private:
	std::map<PBFlowKey, PBFlow, pb_flow_key_less_t> m_map_flow;
	std::map<uint16_t, PBId> m_map_id;
};

class VServClnt
{
public:
	class VServThread : public Thread
	{
	public:

		VServThread(VServClnt *clnt) :
			Thread("vserv_clnt"),
			m_clnt(clnt)
		{}

		void *run()
		{
			BEGIN_DEBUG_EXCEPTION_HANDLER

				m_clnt->threadFunc();

			END_DEBUG_EXCEPTION_HANDLER

				this->stop();

			return nullptr;
		}

	private:
		VServClnt * m_clnt = NULL;
	};

	VServClnt(VServClntCtl *ctl, bool ipv6, uint32_t port, const char *hostname);

	void threadFunc();
	void ident(GsSend *send, const std::string &name_want, const std::string &serv_want, long long timestamp);
	void updateOther(long long timestamp, uint32_t keys);
	void updateRecord(long long timestamp, uint8_t mode, uint16_t blk, const std::string &fra_buf);
	void packetFill(uint8_t *packetdata, size_t packetsize, NetworkPacket *io_packet);
	void processPacket(long long timestamp, uint8_t *packetdata, size_t packetsize, const Address &addr_from);

private:
	VServClntCtl * m_ctl = NULL;

	int16_t m_blk = 0;
	int16_t m_seq = 0;

	GsName m_name;
	std::unique_ptr<GsRenamer> m_renamer;
	std::unique_ptr<GsPinger> m_pinger;
	std::unique_ptr<GsRecord> m_record;
	std::unique_ptr<GsPlayBack> m_playback;

	std::unique_ptr<UDPSocket> m_socket;
	std::unique_ptr<GsSend>    m_send;
	std::unique_ptr<VServThread> m_thread;
	uint32_t m_thread_exit_code;
	Address m_addr;
	std::atomic<uint32_t> m_keys;
	std::mt19937                            m_rand_gen;
	std::uniform_int_distribution<uint32_t> m_rand_dis;
};

#endif /* _VSERV_CLNT_H_ */
