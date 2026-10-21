#pragma once

#if !defined(ARCH_STM32WL)

#include "MeshModule.h"
#include "mesh/ClientBaseSecretaryStore.h"

/**
 * CLIENT_BASE household secretary (L2).
 * P1: ingest secondary TEXT into RAM store (no poll yet - P2 / D1=A).
 */
class ClientBaseSecretaryModule : public MeshModule
{
  public:
    ClientBaseSecretaryModule();

    bool isReady() const { return ready_; }
    ClientBaseSecretaryStore &store() { return store_; }
    const ClientBaseSecretaryStore &store() const { return store_; }

    virtual bool wantPacket(const meshtastic_MeshPacket *p) override;

  protected:
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;

  private:
    ClientBaseSecretaryStore store_;
    bool ready_ = false;
};

extern ClientBaseSecretaryModule *clientBaseSecretaryModule;

#endif // !ARCH_STM32WL
