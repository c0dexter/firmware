#pragma once

#if !defined(ARCH_STM32WL)

#include "ProtobufModule.h"
#include "concurrency/OSThread.h"
#include "mesh/ClientBaseSecretaryPoll.h"
#include "mesh/ClientBaseSecretaryStore.h"
#include "mesh/generated/meshtastic/storeforward.pb.h"

/**
 * CLIENT_BASE household secretary (L2).
 * P1 ingest secondary TEXT; P2 poll via STORE_FORWARD_APP RR (D1=A).
 * Payloads dump as STORE_FORWARD ROUTER_TEXT_* unicast on the stored secondary (hop_limit=0).
 * (Raw TEXT DM on channel PSK is rejected as legacy DM by Router::perhapsDecode.)
 */
class ClientBaseSecretaryModule : private concurrency::OSThread, public ProtobufModule<meshtastic_StoreAndForward>
{
  public:
    ClientBaseSecretaryModule();

    bool isReady() const { return ready_; }
    ClientBaseSecretaryStore &store() { return store_; }
    const ClientBaseSecretaryStore &store() const { return store_; }

    uint32_t statEnqueued() const { return statEnqueued_; }
    uint32_t statPulled() const { return statPulled_; }
    uint32_t statBusy() const { return statBusy_; }
    uint32_t statRejected() const { return statRejected_; }

    virtual bool wantPacket(const meshtastic_MeshPacket *p) override;

  protected:
    virtual int32_t runOnce() override;
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    virtual bool handleReceivedProtobuf(const meshtastic_MeshPacket &mp, meshtastic_StoreAndForward *decoded) override;

  private:
    void sendRr(NodeNum dest, meshtastic_StoreAndForward_RequestResponse rr);
    void sendHistoryHeader(NodeNum dest, uint32_t historyMessages);
    bool sendNextPayload();

    ClientBaseSecretaryStore store_;
    ClientBaseSecretaryPollSession session_;
    bool ready_ = false;

    uint32_t statEnqueued_ = 0;
    uint32_t statPulled_ = 0;
    uint32_t statBusy_ = 0;
    uint32_t statRejected_ = 0;
};

extern ClientBaseSecretaryModule *clientBaseSecretaryModule;

#endif // !ARCH_STM32WL
