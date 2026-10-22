#if !defined(ARCH_STM32WL)

#include "ClientBaseSecretaryModule.h"
#include "Channels.h"
#include "MeshService.h"
#include "MeshTypes.h"
#include "NodeDB.h"
#include "Router.h"
#include "configuration.h"
#include "gps/RTC.h"
#include "mesh-pb-constants.h"
#include "mesh/ClientBaseSecretaryIngest.h"
#include "mesh/ClientBaseSecretaryPolicy.h"

ClientBaseSecretaryModule *clientBaseSecretaryModule;

ClientBaseSecretaryModule::ClientBaseSecretaryModule()
    : concurrency::OSThread("cbaseSec"),
      ProtobufModule("cbaseSec", meshtastic_PortNum_STORE_FORWARD_APP, &meshtastic_StoreAndForward_msg)
{
    const uint16_t cap = clientBaseSecretaryRingCapacity();
    if (cap == 0) {
        LOG_WARN("ClientBaseSecretary: MEM_CLASS too small, store disabled");
        disable();
        return;
    }

    ready_ = store_.init(cap, CLIENT_BASE_SECRETARY_MAX_FAVORITES, CLIENT_BASE_SECRETARY_TTL_SEC);
    if (!ready_) {
        LOG_WARN("ClientBaseSecretary: store init failed (cap=%u)", cap);
        disable();
        return;
    }

    LOG_INFO("ClientBaseSecretary ready: cap=%u favorites=%u ttl=%u s return_max=%u interval=%u ms", cap,
             CLIENT_BASE_SECRETARY_MAX_FAVORITES, CLIENT_BASE_SECRETARY_TTL_SEC, CLIENT_BASE_SECRETARY_HISTORY_RETURN_MAX,
             CLIENT_BASE_SECRETARY_PACKET_INTERVAL_MS);
    disable();
}

bool ClientBaseSecretaryModule::wantPacket(const meshtastic_MeshPacket *p)
{
    if (!ready_ || !p) {
        return false;
    }
    switch (p->decoded.portnum) {
    case meshtastic_PortNum_TEXT_MESSAGE_APP:
    case meshtastic_PortNum_STORE_FORWARD_APP:
        return true;
    default:
        return false;
    }
}

ProcessMessage ClientBaseSecretaryModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!ready_) {
        return ProcessMessage::CONTINUE;
    }

    if (mp.decoded.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP) {
        const bool onSecondary = channels.isPacketOnConfiguredSecondary(mp);
        const uint32_t seq = clientBaseSecretaryIngestText(store_, mp, getTime(), onSecondary, isFromUs(&mp));
        if (seq) {
            statEnqueued_++;
            // Hot path: no LOG_INFO - DEBUG only
            LOG_DEBUG("ClientBaseSecretary ingest seq=%u ch=%u from=0x%08x", seq, mp.channel, getFrom(&mp));
        }
        return ProcessMessage::CONTINUE;
    }

    if (mp.decoded.portnum != meshtastic_PortNum_STORE_FORWARD_APP ||
        mp.which_payload_variant != meshtastic_MeshPacket_decoded_tag) {
        return ProcessMessage::CONTINUE;
    }

    meshtastic_StoreAndForward scratch = meshtastic_StoreAndForward_init_zero;
    if (!pb_decode_from_bytes(mp.decoded.payload.bytes, mp.decoded.payload.size, &meshtastic_StoreAndForward_msg, &scratch)) {
        LOG_ERROR("ClientBaseSecretary: S&F decode failed");
        return ProcessMessage::STOP;
    }
    return handleReceivedProtobuf(mp, &scratch) ? ProcessMessage::STOP : ProcessMessage::CONTINUE;
}

bool ClientBaseSecretaryModule::handleReceivedProtobuf(const meshtastic_MeshPacket &mp, meshtastic_StoreAndForward *decoded)
{
    if (!ready_ || !decoded) {
        return false;
    }

    const NodeNum from = getFrom(&mp);

    switch (decoded->rr) {
    case meshtastic_StoreAndForward_RequestResponse_CLIENT_ABORT:
    case meshtastic_StoreAndForward_RequestResponse_CLIENT_ERROR:
        if (clientBaseSecretaryPollAbort(session_, from)) {
            LOG_INFO("ClientBaseSecretary: poll aborted by 0x%08x", from);
            disable();
        }
        return true;

    case meshtastic_StoreAndForward_RequestResponse_CLIENT_HISTORY: {
        if (!nodeDB || !nodeDB->isFavorite(from)) {
            statRejected_++;
            return true; // silent reject
        }

        const auto start = clientBaseSecretaryBeginPoll(store_, session_, from, true, CLIENT_BASE_SECRETARY_HISTORY_RETURN_MAX);
        switch (start) {
        case ClientBaseSecretaryPollStart::Busy:
            statBusy_++;
            sendRr(from, meshtastic_StoreAndForward_RequestResponse_ROUTER_BUSY);
            LOG_INFO("ClientBaseSecretary: busy enq=%u pulled=%u busy=%u rej=%u", statEnqueued_, statPulled_, statBusy_,
                     statRejected_);
            break;
        case ClientBaseSecretaryPollStart::Empty:
            sendHistoryHeader(from, 0);
            LOG_INFO("ClientBaseSecretary: no history for 0x%08x", from);
            break;
        case ClientBaseSecretaryPollStart::Started:
            sendHistoryHeader(from, session_.remaining);
            LOG_INFO("ClientBaseSecretary: dump %u msg to 0x%08x (enq=%u)", session_.remaining, from, statEnqueued_);
            // Ctor leave-disabled: setIntervalFromNow alone does not arm Thread::enabled.
            enabled = true;
            setIntervalFromNow(CLIENT_BASE_SECRETARY_PACKET_INTERVAL_MS);
            break;
        case ClientBaseSecretaryPollStart::Rejected:
            statRejected_++;
            break;
        default:
            break;
        }
        return true;
    }

    default:
        return false;
    }
}

void ClientBaseSecretaryModule::sendRr(NodeNum dest, meshtastic_StoreAndForward_RequestResponse rr)
{
    meshtastic_StoreAndForward sf = meshtastic_StoreAndForward_init_zero;
    sf.rr = rr;
    meshtastic_MeshPacket *p = allocDataProtobuf(sf);
    if (!p || !service) {
        if (p) {
            packetPool.release(p);
        }
        return;
    }
    p->to = dest;
    p->priority = meshtastic_MeshPacket_Priority_BACKGROUND;
    p->want_ack = false;
    p->hop_limit = 0;
    service->sendToMesh(p);
}

void ClientBaseSecretaryModule::sendHistoryHeader(NodeNum dest, uint32_t historyMessages)
{
    meshtastic_StoreAndForward sf = meshtastic_StoreAndForward_init_zero;
    sf.rr = meshtastic_StoreAndForward_RequestResponse_ROUTER_HISTORY;
    sf.which_variant = meshtastic_StoreAndForward_history_tag;
    sf.variant.history.history_messages = historyMessages;
    sf.variant.history.window = 0;
    sf.variant.history.last_request = 0;
    meshtastic_MeshPacket *p = allocDataProtobuf(sf);
    if (!p || !service) {
        if (p) {
            packetPool.release(p);
        }
        return;
    }
    p->to = dest;
    p->priority = meshtastic_MeshPacket_Priority_BACKGROUND;
    p->want_ack = false;
    p->hop_limit = 0;
    service->sendToMesh(p);
}

bool ClientBaseSecretaryModule::sendNextPayload()
{
    if (!session_.busy || !router || !service) {
        return false;
    }

    const NodeNum dest = session_.busyTo;
    ClientBaseLeanRecord rec;
    // Skip requester's own TEXTs (same as classic S&F); desk 2-node seeds are often from==dest.
    while (true) {
        if (!store_.peekNext(dest, rec)) {
            clientBaseSecretaryPollClear(session_);
            return false;
        }
        if (rec.from != dest) {
            break;
        }
        ClientBaseLeanRecord skip;
        if (!clientBaseSecretaryPollPullNext(store_, session_, skip)) {
            return false;
        }
        if (!session_.busy) {
            return false;
        }
    }

    meshtastic_MeshPacket *p = router->allocForSending();
    if (!p) {
        LOG_WARN("ClientBaseSecretary: allocForSending failed");
        return true;
    }

    p->to = dest;
    p->from = rec.from;
    // Fresh PacketId: reusing rec.id is a dupe if the favorite already heard the live TX.
    // Do not send channel-PSK TEXT unicasts: Router rejects them as "legacy DM". Match
    // classic StoreForwardModule: STORE_FORWARD ROUTER_TEXT_* wrapper (forged from OK).
    p->channel = rec.channel;
    p->want_ack = false;
    p->hop_limit = 0;
    p->hop_start = 0;
    // Forged from skips Router::send's isFromUs hop_start/bitfield init; without has_bitfield,
    // receivers treat 0/0 as pre-hop firmware and post-decode drop (never reach the phone).
    p->decoded.has_bitfield = true;
    p->priority = meshtastic_MeshPacket_Priority_BACKGROUND;

    meshtastic_StoreAndForward sf = meshtastic_StoreAndForward_init_zero;
    sf.rr = meshtastic_StoreAndForward_RequestResponse_ROUTER_TEXT_BROADCAST;
    sf.which_variant = meshtastic_StoreAndForward_text_tag;
    const size_t textMax = sizeof(sf.variant.text.bytes);
    const size_t n = rec.payload_len > textMax ? textMax : rec.payload_len;
    sf.variant.text.size = n;
    if (n) {
        memcpy(sf.variant.text.bytes, rec.payload, n);
    }
    if (rec.id) {
        sf.original_id = rec.id;
    }
    p->decoded.portnum = meshtastic_PortNum_STORE_FORWARD_APP;
    p->decoded.payload.size =
        pb_encode_to_bytes(p->decoded.payload.bytes, sizeof(p->decoded.payload.bytes), &meshtastic_StoreAndForward_msg, &sf);

    ClientBaseLeanRecord advanced;
    if (!clientBaseSecretaryPollPullNext(store_, session_, advanced)) {
        packetPool.release(p);
        return false;
    }

    statPulled_++;
    service->sendToMesh(p);
    return session_.busy;
}

int32_t ClientBaseSecretaryModule::runOnce()
{
    if (!ready_ || !session_.busy) {
        return disable();
    }

    if (!sendNextPayload()) {
        LOG_INFO("ClientBaseSecretary: dump done sent=%u pulled=%u enq=%u", session_.sent, statPulled_, statEnqueued_);
        return disable();
    }
    return CLIENT_BASE_SECRETARY_PACKET_INTERVAL_MS;
}

#endif // !ARCH_STM32WL
