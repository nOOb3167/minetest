#include <cassert>
#include <cstdint>

#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include <network/address.h>
#include <network/networkpacket.h>
#include <network/socket.h>
#include <porting.h>
#include <threading/thread.h>
#include <util/basic_macros.h>

#include <client/vserv/ns_vserv_clnt.h>
#include <client/vserv/ns_vserv_clnt_iface.h>
#include <client/vserv/ns_vserv_openal_include.h>

// FIXME: defined here for now
VServClntCtl *g_vserv_clnt_ctl = NULL;

// FIXME: temp global data
static uint8_t g_buf_silence_dummy[2 * (GS_48KHZ / 1000 * GS_OPUS_FRAME_DURATION_20MS)] = {};

// FIXME: duplicated from sound_openal.cpp
static const char *alErrorString(ALenum err)
{
	switch (err) {
	case AL_NO_ERROR:
		return "no error";
	case AL_INVALID_NAME:
		return "invalid name";
	case AL_INVALID_ENUM:
		return "invalid enum";
	case AL_INVALID_VALUE:
		return "invalid value";
	case AL_INVALID_OPERATION:
		return "invalid operation";
	case AL_OUT_OF_MEMORY:
		return "out of memory";
	default:
		return "<unknown OpenAL error>";
	}
}

// FIXME: duplicated from sound_openal.cpp
static ALenum warn_if_error(ALenum err, const char *desc)
{
	if (err == AL_NO_ERROR)
		return err;
	warningstream << desc << ": " << alErrorString(err) << std::endl;
	return err;
}

GsRenamer::GsRenamer(const std::string &name_want, const std::string &serv_want, long long timestamp, uint32_t rand):
	m_name_want(name_want),
	m_serv_want(serv_want),
	m_timestamp_last_requested(timestamp),
	m_rand_last_requested(rand)
{}

bool GsRenamer::matchingRand(uint32_t rand)
{
	return m_rand_last_requested == rand;
}

bool GsRenamer::isWanted()
{
	assert(m_name_want.empty() == m_serv_want.empty());
	return !m_name_want.empty() && !m_serv_want.empty();
}

void GsRenamer::identEmit(NetworkPacket *packet)
{
	*packet << (uint8_t)GS_VSERV_CMD_IDENT << m_rand_last_requested << (uint32_t) m_name_want.size() << (uint32_t) m_serv_want.size();
	packet->putRawString(m_name_want);
	packet->putRawString(m_serv_want);
}

void GsRenamer::update(GsSend *send, long long timestamp)
{
	/* no update work needed at all */
	if (!isWanted())
		return;

	/* update work needed - but not yet */
	if (timestamp < m_timestamp_last_requested + GS_CLNT_ARBITRARY_IDENT_RESEND_TIMEOUT)
		return;

	NetworkPacket packet;

	identEmit(&packet);

	send->send(&packet);

	m_timestamp_last_requested = timestamp;
}

void GsRenamer::reset()
{
	m_name_want.clear();
	m_serv_want.clear();
}

GsName GsRenamer::wantedName(uint16_t id)
{
	GsName name;
	name.m_name = m_name_want;
	name.m_serv = m_serv_want;
	name.m_id = id;
	return name;
}

void GsPinger::update(GsSend *send, long long timestamp)
{
	if (timestamp < m_timestamp_last_ping + GS_PINGER_REQUEST_INTERVAL_MS)
		return;

	NetworkPacket packet;

	packet << (uint8_t) GS_VSERV_CMD_PING;

	send->send(&packet);

	m_timestamp_last_ping = timestamp;
}

GsRecord::GsRecord():
	// FIXME: configurable device selection wanted
	m_cap_device(alcCaptureOpenDevice(NULL, GS_48KHZ, AL_FORMAT_MONO16, GS_RECORD_ARBITRARY_BUFFER_SAMPLES_NUM), deleteDevice),
	m_opusenc(opus_encoder_create(GS_48KHZ, 1, OPUS_APPLICATION_VOIP, NULL), deleteOpusEnc),
	m_blk(0),
	m_seq(0)
{
	if (! m_cap_device)
		throw std::runtime_error("OpenAL capture open");
	if (! m_opusenc)
		throw std::runtime_error("Opus encoder create");
}

void GsRecord::start()
{
	alcCaptureStart(m_cap_device.get());
	warn_if_error(alGetError(), "OpenAL capture start");
}

void GsRecord::stop()
{
	alcCaptureStop(m_cap_device.get());
	warn_if_error(alGetError(), "OpenAL capture stop");
}

void GsRecord::captureDrain(size_t SampSize, size_t FraNumSamp, std::string *FraBuf)
{
	/*
	* OpenAL alcGetIntegerv exposes the count of new samples arrived, but not yet delivered via alcCaptureSamples.
	*   this count grows during an ongoing capture (ex started via alcCaptureStart).
	* we want samples delivered in blocks of certain size.
	*   (specifically blocks of size of an Opus frame, see calculation of 'OpFraNumSamp'.
	*    Opus supports only specific Opus frame sizes, 20ms hardcoded.
	*    https://wiki.xiph.org/Opus_Recommended_Settings : Quote "Opus can encode frames of 2.5, 5, 10, 20, 40, or 60 ms.")
	* if this function keeps getting called during an ongoing capture, alcGetIntegerv eventually will report enough samples to fill one or more Opus frames.
	*/

	// FIXME: apply fix once opus becomes used
	typedef ALshort should_be_opus_int16_tho;

	/* the values are hardcoded for 1 channel (mono) (ex for 2 channels a 'sample' is actually two individual ALshort or opus_int16 values) */
	const size_t AlSampSize = sizeof(ALshort); /*AL_FORMAT_MONO16*/
	const size_t OpSampSize = sizeof(should_be_opus_int16_tho); /*opus_encode API doc*/
	const size_t OpFraNumSamp = GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM;
	const size_t OpFraSize = OpFraNumSamp * OpSampSize;
	assert(SampSize == OpSampSize && OpSampSize == AlSampSize);
	assert(FraNumSamp == OpFraNumSamp);

	ALCint NumAvailSamp = 0;
	size_t NumAvailFraAl = 0;
	size_t NumAvailFraBuf = 0;
	size_t NumFraToProcess = 0;

	alcGetIntegerv(m_cap_device.get(), ALC_CAPTURE_SAMPLES, 1, &NumAvailSamp);

	warn_if_error(alGetError(), "OpenAL capture samples query");

	NumAvailFraAl = (NumAvailSamp / OpFraNumSamp); /*truncating division*/

	if (NumAvailFraAl == 0)
		return;

	FraBuf->resize(1 * OpFraSize);

	NumAvailFraBuf = (FraBuf->size() / OpFraSize);     /*truncating division*/
	NumFraToProcess = MYMIN(NumAvailFraAl, NumAvailFraBuf);

	assert(NumFraToProcess == 1);

	/* capture a single frame (call this function in a loop..) */

	alcCaptureSamples(m_cap_device.get(), (void *) FraBuf->data(), 1 * OpFraNumSamp);

	warn_if_error(alGetError(), "OpenAL capture samples");
}

void GsRecord::resetForBlockConditionally(uint16_t blk)
{
	if (m_blk != blk) {
		if (! (m_opusenc = std::move(unique_ptr_opusenc(opus_encoder_create(GS_48KHZ, 1, OPUS_APPLICATION_VOIP, NULL), deleteOpusEnc))))
			throw std::runtime_error("Opus encoder create");
		m_blk = blk;
		m_seq = 0;
	}
}

std::string GsRecord::encodeFrame(const std::string &fra_buf)
{
	std::string opusfra_buf;

	assert(fra_buf.size() == GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM * 1 * sizeof(opus_int16));

	opusfra_buf.resize(GS_OPUS_FRAME_ENCODED_MAX_DATA_BYTES);

	int len = opus_encode(m_opusenc.get(),
			(opus_int16    *) fra_buf.data()    , GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM,
			(unsigned char *) opusfra_buf.data(), opusfra_buf.size());
	if (len < 0)
		throw std::runtime_error("Opus encoder encode");

	opusfra_buf.resize(len);

	return opusfra_buf;
}

void GsRecord::advanceSeq()
{
	// FIXME: overflow handling (maybe resetForBlockConditionally(m_blk+1))
	m_seq += 1;
}

void GsRecord::deleteDevice(ALCdevice *device)
{
	if (device)
		if (!alcCaptureCloseDevice(device))
			assert(0);
}

void GsRecord::deleteOpusEnc(OpusEncoder *enc)
{
	if (enc)
		opus_encoder_destroy(enc);
}

GsPlayBack::PBFlow::PBFlow(GsPlayBack::unique_ptr_aluint source, long long timestamp_first_receipt):
		m_source(std::move(source)),
		m_opusdec(opus_decoder_create(GS_48KHZ, 1, NULL), deleteOpusDec),
		m_timestamp_first_receipt(timestamp_first_receipt),
		m_next_seq(0)
{
	if (!m_opusdec)
		throw std::runtime_error("Opus decoder create");
}

std::string GsPlayBack::PBFlow::decodeFrame(const std::string &fra_buf)
{
	std::string opusfra_buf;

	/* see doc of opus_decode about the function being unable to decode some packets
	   unless its 'pcm' and 'frame_size' parameters are big enough */

	opusfra_buf.resize(GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM * 1 * sizeof(opus_int16));

	int num_samp = opus_decode(m_opusdec.get(),
			(const unsigned char *) fra_buf.data(), fra_buf.size(),
			(opus_int16 *) opusfra_buf.data(), GS_OPUS_FRAME_48KHZ_120MS_SAMP_NUM,
			0);
	if (num_samp < 0)
		throw std::runtime_error("Opus decoder decode");

	/* https://wiki.xiph.org/OpusFAQ#What_frame_size_should_I_use.3F
	   even though we decode with capacity to receive up to 120ms packets,
	   the code works with 20ms packets */
	assert(num_samp == GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM);

	opusfra_buf.resize(num_samp * 1 * sizeof(opus_int16));

	return opusfra_buf;
}

std::string GsPlayBack::PBFlow::decodeFrameMissing()
{
	std::string opusfra_buf;

	/* see doc of opus_decode about how frame_size is constrained for PLC (data=NULL).
	   frame_size must reflect duration of missing audio.
	   as the code works with 20ms packets, there is 20ms of missing audio. */

	opusfra_buf.resize(GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM * 1 * sizeof(opus_int16));

	int num_samp = opus_decode(m_opusdec.get(),
			NULL, 0,
			(opus_int16 *) opusfra_buf.data(), GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM,
			0);
	if (num_samp < 0)
		throw std::runtime_error("Opus decoder decode");

	assert(num_samp == GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM);

	return opusfra_buf;
}

GsPlayBack::GsPlayBack() = default;

void GsPlayBack::packetInsert(long long timestamp, uint16_t id, uint16_t blk, uint16_t seq,
		const char *data, size_t len_data)
{
	const PBFlowKey key = { id, blk };

	auto itId = m_map_id.find(id);

	if (itId == m_map_id.end())
		itId = (m_map_id.insert(std::make_pair(id, PBId()))).first;

	if (blk < itId->second.m_blk_floor)
		return;

	{
		auto it1 = m_map_flow.find(key);

		if (it1 == m_map_flow.end()) {
			/* this is the first packet seen from this flow */
			if (seq > GS_PLAYBACK_FLOW_LEADING_SEQUENCE_LOSS_THRESHOLD) {
				/* if such a first packet arrives with a high Seq number
					we can assume the earlier Seq got lost, somehow otherwise will not be arriving,
					or at the very least be severely outdated the moment they would of been received.
					ATM the known non-failure path known to cause such high Seq number is
					a client connecting to the voice server during an ongoing long-running
					flow. The client, having just connected, is guaranteed to miss the
					earlier Seq. First packet received of the long-running flow will have high Seq.
					Two ways of handling such high Seq scenarios were considered:
					    - play the flow as usual (low Seq considered packet-loss)
					    leads to a severe time lag between recording and eventual playback of the affected flow.
					    - drop the flow
					    ''cant have time lag if you aint ever playing the flow :>''
					    potentially discards perfectly usable data (rest of the long-running flow)
					Current handling implemented as drop the flow.
				*/
				return;
			}
			else {
				unique_ptr_aluint source(new ALuint(-1), deleteSource);
				alGenSources(1, source.get());
				alSourcef(*source, AL_MIN_GAIN, 1.0f);
				alSourcef(*source, AL_MAX_GAIN, 1.0f);
				warn_if_error(alGetError(), "PlayBack source creation");
				it1 = (m_map_flow.insert(std::make_pair(key, PBFlow(std::move(source), timestamp)))).first;
			}
		}

		auto it2 = it1->second.m_map_buf.find(seq);

		if (it2 == it1->second.m_map_buf.end()) {
			std::string buf(data, len_data);
			it2 = (it1->second.m_map_buf.insert(std::make_pair(seq, std::move(buf)))).first;
		}
	}
}

void GsPlayBack::harvest(long long timestamp, std::vector<std::pair<ALuint, unique_ptr_aluint> > *out_buffer_vec)
{
	out_buffer_vec->clear();
	for (auto itFlow = m_map_flow.begin(); itFlow != m_map_flow.end(); ++itFlow) {
		const long long flow_playback_start_time = itFlow->second.m_timestamp_first_receipt + GS_PLAYBACK_FLOW_DELAY_MS;
		if (timestamp < flow_playback_start_time)
			continue;
		const uint16_t seq_current_time = (timestamp - flow_playback_start_time) / GS_OPUS_FRAME_DURATION_20MS;
		assert(itFlow->second.m_next_seq <= seq_current_time);
		const size_t count = seq_current_time - itFlow->second.m_next_seq;
		for (size_t j = 0; j < count; j++) {
			/* will emit a (ALsource, ALbuffer) pair */
			unique_ptr_aluint buffer(new ALuint(-1), deleteBuffer);
			alGenBuffers(1, buffer.get());
			warn_if_error(alGetError(), "PlayBack buffer creation");
			/* buffer contents depending on whether we received the data - or are compensating packet loss etc */
			auto itBuf = itFlow->second.m_map_buf.find(itFlow->second.m_next_seq + j);
			std::string opusfra_buf;
			if (itBuf != itFlow->second.m_map_buf.end())
				opusfra_buf = itFlow->second.decodeFrame(itBuf->second);
			else
				opusfra_buf = itFlow->second.decodeFrameMissing();
			alBufferData(*buffer, AL_FORMAT_MONO16, opusfra_buf.data(), opusfra_buf.size(), GS_48KHZ);
			warn_if_error(alGetError(), "PlayBack buffer data");
			/* visualization / debugging */
			g_vserv_clnt_ctl->s_enqueueframehud2(g_vserv_clnt_ctl, opusfra_buf);
			/* emit */
			out_buffer_vec->push_back(std::make_pair(*itFlow->second.m_source, std::move(buffer)));
		}
		itFlow->second.m_next_seq += count;
	}
}

void GsPlayBack::harvestAndEnqueue(long long timestamp)
{
	std::vector<std::pair<ALuint, unique_ptr_aluint> > buffer_vec;

	harvest(timestamp, &buffer_vec);

	for (size_t i = 0; i < buffer_vec.size(); i++) {
		alSourceQueueBuffers(buffer_vec[i].first, 1, buffer_vec[i].second.get());
		/* if the buffer was queued successfully we will take care of its deletion
		   (via alDeleteBuffers) after unqueuing - here just delete memory for the ALuint. */
		if (alGetError() == AL_NO_ERROR)
			delete buffer_vec[i].second.release();
		warn_if_error(alGetError(), "PlayBack source buffer queue");
	}
}

void GsPlayBack::dequeue()
{
	for (auto itFlow = m_map_flow.begin(); itFlow != m_map_flow.end(); ++itFlow)
		dequeueOne(*itFlow->second.m_source);
}

void GsPlayBack::dequeueOne(ALuint source)
{
	ALint num_processed = 0;
	do {
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &num_processed);
		warn_if_error(alGetError(), "PlayBack get buffers processed");
		if (num_processed > 0) {
			ALuint bufferDummy[16];
			alSourceUnqueueBuffers(source, MYMIN(num_processed, 16), bufferDummy);
			warn_if_error(alGetError(), "PlayBack get buffers processed");
			/* OpenAL allows queuing an ALbuffer to multiple sources.
			   this is not used therefore having a buffer unqueued is enough confirmation
			   to perform a delete safely. */
			alDeleteBuffers(MYMIN(num_processed, 16), bufferDummy);
		}
	} while (num_processed);
}

void GsPlayBack::ensurePlaying()
{
	for (auto itFlow = m_map_flow.begin(); itFlow != m_map_flow.end(); ++itFlow) {
		ALint state = 0;
		alGetSourcei(*itFlow->second.m_source, AL_SOURCE_STATE, &state);
		warn_if_error(alGetError(), "PlayBack get source state");
		if (state != AL_PLAYING)
			alSourcePlay(*itFlow->second.m_source);
		warn_if_error(alGetError(), "PlayBack source play");
	}
}

void GsPlayBack::ensureStoppedOne(ALuint source)
{
	alSourceStop(source);
	warn_if_error(alGetError(), "PlayBack source stop");
}

void GsPlayBack::expireFlows(long long timestamp)
{
	for (auto itFlow = m_map_flow.begin(); itFlow != m_map_flow.end(); /*dummy*/) {
		if (isLiveFlow(itFlow->first, timestamp)) {
			/*keep*/
			++itFlow;
		}
		else {
			/*drop*/
			itFlow = m_map_flow.erase(itFlow);
			// FIXME: the correct time for removal of mMapId[Id] is after disconnection of user Id,
			//        and before Id gets recycled for a new connection.
			//        ex even if all flows with keys mId=Id are gone from mMapFlow are gone
			//          more such flows could arrive in the future. if mMapId[Id] were removed,
			//          its former state would be lost.
			//// m_map_id.erase(itFlow->first.m_id);
		}
	}
}

bool GsPlayBack::isLiveFlow(const PBFlowKey &key, long long timestamp)
{
	auto itFlow = m_map_flow.find(key);
	/* cant be alive if it aint there */
	if (itFlow == m_map_flow.end())
		return false;
	const long long flow_playback_start_time = itFlow->second.m_timestamp_first_receipt + GS_PLAYBACK_FLOW_DELAY_MS;
	/* cant be dead it if it aint ever even given a chance to get goin */
	if (timestamp < flow_playback_start_time)
		return true;
	const uint16_t seq_current_time = (timestamp - flow_playback_start_time) / GS_OPUS_FRAME_DURATION_20MS;
	const auto itLastReceived = itFlow->second.m_map_buf.rbegin();
	/* check if the flow expired (ie we postulate further packets will either not arrive,
		or arrive and be too late (ex if we're past 5s into playing a flow and receive a packet
		carrying data from 1 to 1.050s it is too late to play).

		currently the code count a flow as expired if:
		    - we are (time-wise) sufficiently past the last arrived/received packet in the flow
		    - or, should none have arrived, we are sufficiently past the playback start time of that flow
		sufficiently behind meaning GS_PLAYBACK_FLOW_DELAY_EXPIRY_MS or more msec of delay
	*/
	long long expiry_comparison_start_time = flow_playback_start_time;
	if (itLastReceived != itFlow->second.m_map_buf.rend()) {
		const uint16_t  seq_last_received = itLastReceived->first;
		const long long seq_last_received_start_time_offset = seq_last_received * GS_OPUS_FRAME_DURATION_20MS;
		expiry_comparison_start_time += seq_last_received_start_time_offset;
	}
	expiry_comparison_start_time += GS_PLAYBACK_FLOW_DELAY_EXPIRY_MS;
	if (expiry_comparison_start_time < timestamp)
		return false;
	return true;
}

void GsPlayBack::deleteBuffer(ALuint *buffer)
{
	if (*buffer != -1)
		alDeleteBuffers(1, buffer);
	delete buffer;
}

void GsPlayBack::deleteSource(ALuint *source)
{
	/* the source deleter needs to take care of having it:
	     stop, dequeue and delete buffers, delete itself. */
	if (*source != -1) {
		ensureStoppedOne(*source);
		dequeueOne(*source);

		alDeleteSources(1, source);
		warn_if_error(alGetError(), "PlayBack source delete");
	}
	delete source;
}

void GsPlayBack::deleteOpusDec(OpusDecoder *dec)
{
	if (dec)
		opus_decoder_destroy(dec);
}

VServClnt::VServClnt(VServClntCtl *ctl, bool ipv6, uint32_t port, const char *hostname):
	m_ctl(ctl),
	m_name(),
	m_renamer(new GsRenamer()),
	m_pinger(new GsPinger()),
	m_record(new GsRecord()),
	m_playback(new GsPlayBack()),
	m_socket(new UDPSocket(ipv6)),
	m_send(),
	m_thread(new VServThread(this)),
	m_thread_exit_code(0),
	m_addr(0, 0, 0, 0, port),
	m_keys(0),
	m_rand_gen(std::random_device()()),
	m_rand_dis(std::uniform_int_distribution<uint32_t>())
{
	assert(! ipv6);
	m_socket->Bind(Address((irr::u32)INADDR_ANY, 0));
	m_addr.Resolve(hostname);
	m_send.reset(new GsSend(m_socket.get(), m_addr));
	m_thread->start();
}

void VServClnt::threadFunc()
{
	{
		while (!m_ctl->msgHas())
			sleep_ms(100);
		VServClntMsg msg = m_ctl->msgPop();
		assert(msg.m_type == VSERV_CLNT_MSG_TYPE_NAME);
		ident(m_send.get(), msg.m_name.m_name, msg.m_name.m_serv, porting::getTimeMs());
	}

	long long timestamp_last_run = porting::getTimeMs();

	while (! m_thread->stopRequested()) {
		uint32_t keys = 0;
		long long timestamp_before_wait = porting::getTimeMs();
		bool wait_indicates_data_arrived = 0;
		if (timestamp_before_wait < timestamp_last_run) /* backwards clock? wtf? */
			timestamp_before_wait = LLONG_MAX;          /* just ensure processing runs immediately */
		long long time_remaining_to_full_tick = GS_CLNT_ONE_TICK_MS - MYMIN(timestamp_before_wait - timestamp_last_run, GS_CLNT_ONE_TICK_MS);
		wait_indicates_data_arrived = m_socket->WaitData(time_remaining_to_full_tick); /* note indication is not actually used */
		timestamp_last_run = porting::getTimeMs();
		updateOther(timestamp_last_run, m_keys.load());
	}
}

void VServClnt::ident(GsSend *send, const std::string &name_want, const std::string &serv_want, long long timestamp)
{
	NetworkPacket packet;
	uint32_t fresh_rand = m_rand_dis(m_rand_gen);
	std::unique_ptr<GsRenamer> renamer(new GsRenamer(name_want, serv_want, timestamp, fresh_rand));
	renamer->identEmit(&packet);
	send->send(&packet);
	m_renamer = std::move(renamer);
}

void VServClnt::updateOther(long long timestamp, uint32_t keys)
{
	Address addr;

	/* recording and network sending of recorded sound data */

	// FIXME: temporary testing dummy
	m_record->start();

	while (true) {
		std::string fra_buf;
		m_record->captureDrain(sizeof (uint16_t), GS_OPUS_FRAME_48KHZ_20MS_SAMP_NUM, &fra_buf);
		if (fra_buf.empty())
			break;
		uint8_t mode = (keys >> 0) & 0xFF;
		uint16_t blk = (keys >> 8) & 0xFFFF;
		updateRecord(timestamp, mode, blk, fra_buf);
		/* visualization / debugging */
		m_ctl->s_enqueueframehud(m_ctl, std::move(fra_buf));
	}

	/* network receiving and general network processing
	        receives sound data (ex accumulates playback with gs_playback_packet_insert) */

	m_pinger->update(m_send.get(), timestamp);
	m_renamer->update(m_send.get(), timestamp);

	while (true) {
		Address addr_from;
		// the comment below was copied from connectionthreads.cpp as rationale for packet size
		//   use IPv6 minimum allowed MTU as receive buffer size as this is
		//   theoretical reliable upper boundary of a udp packet for all IPv6 enabled
		//   infrastructure
		//const size_t packet_maxsize = 1500;
		// FIXME: actually nothing wrong with big _incoming_ packet size limits
		const size_t packet_maxsize = 8192;
		uint8_t packetdata[packet_maxsize];
		size_t packetsize = 0;
		if (-1 == (packetsize = m_socket->Receive(addr_from, packetdata, packet_maxsize)))
			break;
		processPacket(timestamp, packetdata, packetsize, addr_from);
	}

	/* processing of received and accumulated playback */

	m_playback->dequeue();
	m_playback->expireFlows(timestamp);
	m_playback->harvestAndEnqueue(timestamp);
	m_playback->ensurePlaying();
}

void VServClnt::updateRecord(long long timestamp, uint8_t mode, uint16_t blk, const std::string &fra_buf)
{
	/* no mode - no send requested */
	if (mode == GS_VSERV_GROUP_MODE_NONE)
		return;

	/* fresh blk? */
	m_record->resetForBlockConditionally(blk);

	std::string opusfra_buf = m_record->encodeFrame(fra_buf);

	/* commit mSeq */
	m_record->advanceSeq();

	NetworkPacket packet;

	packet << (uint8_t) GS_VSERV_CMD_GROUP_MODE_MSG << mode << m_name.m_id << m_record->getBlk() << m_record->getSeq();
	packet.putRawString(opusfra_buf);

	m_send->send(&packet);
}

void VServClnt::packetFill(uint8_t *packetdata, size_t packetsize, NetworkPacket *io_packet)
{
	// FIXME: inefficient
	std::unique_ptr<uint8_t[]> pp(new uint8_t[2 + packetsize]);
	pp[0] = 0; pp[1] = 0;
	memcpy(pp.get() + 2, packetdata, packetsize);
	io_packet->putRawPacket(pp.get(), 2 + packetsize, -1);
}

void VServClnt::processPacket(long long timestamp, uint8_t *packetdata, size_t packetsize, const Address &addr_from)
{
	NetworkPacket packet;

	packetFill(packetdata, packetsize, &packet);

	uint8_t cmd = 0;

	packet >> cmd;

	switch (cmd) {
	case GS_VSERV_CMD_IDENT_ACK:
	{
		uint32_t rand = 0;
		uint16_t id = GS_VSERV_USER_ID_SERVFILL;

		packet >> rand >> id;

		/* unsolicited or reliability-codepath (ex re-sent or reordered packet) GS_VSERV_CMD_IDENT_ACK */

		if (! m_renamer->isWanted() || ! m_renamer->matchingRand(rand))
			break;

		/* seems legit, apply and reset */

		m_name = m_renamer->wantedName(id);
		m_renamer->reset();
	}
	break;

	case GS_VSERV_CMD_GROUP_MODE_MSG:
	{
		uint8_t mode = 0;
		uint16_t id = 0;
		uint16_t blk = 0;
		uint16_t seq = 0;

		packet >> mode >> id >> blk >> seq;

		// FIXME: hmmm but SERVFILL_FIXME is actually a valid uint16_t value / id ?
		//   prevent generating those (fix ex gs_vserv_user_genid)
		assert(id != GS_VSERV_USER_ID_SERVFILL);

		m_playback->packetInsert(timestamp, id, blk, seq, packet.getRemainingString(), packet.getRemainingBytes());
	}
	break;

	default:
		assert(0);
	}
}
