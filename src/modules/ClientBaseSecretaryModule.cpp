#if !defined(ARCH_STM32WL)

#include "ClientBaseSecretaryModule.h"
#include "Channels.h"
#include "MeshTypes.h"
#include "configuration.h"
#include "gps/RTC.h"
#include "mesh/ClientBaseSecretaryIngest.h"
#include "mesh/ClientBaseSecretaryPolicy.h"

ClientBaseSecretaryModule *clientBaseSecretaryModule;

ClientBaseSecretaryModule::ClientBaseSecretaryModule() : MeshModule("cbaseSec")
{
    const uint16_t cap = clientBaseSecretaryRingCapacity();
    if (cap == 0) {
        LOG_WARN("ClientBaseSecretary: MEM_CLASS too small, store disabled");
        return;
    }

    ready_ = store_.init(cap, CLIENT_BASE_SECRETARY_MAX_FAVORITES, CLIENT_BASE_SECRETARY_TTL_SEC);
    if (!ready_) {
        LOG_WARN("ClientBaseSecretary: store init failed (cap=%u)", cap);
        return;
    }

    LOG_INFO("ClientBaseSecretary ready: cap=%u favorites=%u ttl=%u s", cap, CLIENT_BASE_SECRETARY_MAX_FAVORITES,
             CLIENT_BASE_SECRETARY_TTL_SEC);
}

bool ClientBaseSecretaryModule::wantPacket(const meshtastic_MeshPacket *p)
{
    return ready_ && p && p->decoded.portnum == meshtastic_PortNum_TEXT_MESSAGE_APP;
}

ProcessMessage ClientBaseSecretaryModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!ready_) {
        return ProcessMessage::CONTINUE;
    }

    const bool onSecondary = channels.isPacketOnConfiguredSecondary(mp);
    const uint32_t seq = clientBaseSecretaryIngestText(store_, mp, getTime(), onSecondary, isFromUs(&mp));
    if (seq) {
        LOG_DEBUG("ClientBaseSecretary ingest seq=%u ch=%u from=0x%08x", seq, mp.channel, getFrom(&mp));
    }

    // Never steal TEXT from TextMessageModule / phone path.
    return ProcessMessage::CONTINUE;
}

#endif // !ARCH_STM32WL
