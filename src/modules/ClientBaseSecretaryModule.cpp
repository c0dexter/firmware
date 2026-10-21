#if !defined(ARCH_STM32WL)

#include "ClientBaseSecretaryModule.h"
#include "configuration.h"
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

ProcessMessage ClientBaseSecretaryModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    (void)mp;
    // P0: wantPacket is false - this path should not run. Keep CONTINUE if it does.
    return ProcessMessage::CONTINUE;
}

#endif // !ARCH_STM32WL
