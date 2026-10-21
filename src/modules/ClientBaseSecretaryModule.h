#pragma once

#if !defined(ARCH_STM32WL)

#include "MeshModule.h"
#include "mesh/ClientBaseSecretaryStore.h"

/**
 * CLIENT_BASE household secretary (L2).
 * P0: owns RAM store + D2 caps; no ingest/poll yet (P1/P2).
 * D1: poll will reuse STORE_FORWARD_APP RR when wired - not classic S&F server.
 */
class ClientBaseSecretaryModule : public MeshModule
{
  public:
    ClientBaseSecretaryModule();

    bool isReady() const { return ready_; }
    ClientBaseSecretaryStore &store() { return store_; }
    const ClientBaseSecretaryStore &store() const { return store_; }

    /** P0: ignore all ports until P1 ingest / P2 poll. */
    virtual bool wantPacket(const meshtastic_MeshPacket *p) override { return false; }

  protected:
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;

  private:
    ClientBaseSecretaryStore store_;
    bool ready_ = false;
};

extern ClientBaseSecretaryModule *clientBaseSecretaryModule;

#endif // !ARCH_STM32WL
