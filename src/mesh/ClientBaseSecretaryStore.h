#pragma once

#include "ClientBaseSecretaryRing.h"
#include "MeshTypes.h"
#include <cstdint>

/**
 * Shared ring + per-favorite cursor store for CLIENT_BASE L2.
 * Advancing one client's cursor never deletes the ring entry (no purge-on-ACK).
 * Ring storage is pluggable (L2-P4.1); default backend is RAM.
 */
class ClientBaseSecretaryStore
{
  public:
    ClientBaseSecretaryStore() = default;
    ~ClientBaseSecretaryStore() { freeStorage(); }

    ClientBaseSecretaryStore(const ClientBaseSecretaryStore &) = delete;
    ClientBaseSecretaryStore &operator=(const ClientBaseSecretaryStore &) = delete;

    /** Allocate ring + cursor slots. Returns false on OOM or bad args. */
    bool init(uint16_t capacity, uint8_t maxFavorites, uint32_t ttlSec);

    void clear();

    uint16_t capacity() const { return ring_ ? ring_->capacity() : 0; }
    uint16_t size() const { return ring_ ? ring_->size() : 0; }
    uint8_t maxFavorites() const { return maxFavorites_; }
    uint32_t ttlSec() const { return ttlSec_; }
    uint32_t nextSeq() const { return ring_ ? ring_->nextSeq() : 1; }

    uint32_t push(const ClientBaseLeanRecord &in, uint32_t nowSec);
    void expire(uint32_t nowSec);

    bool ensureFavorite(NodeNum node);
    bool hasFavorite(NodeNum node) const;

    uint16_t availableCount(NodeNum node) const;
    uint16_t availableCountNotFrom(NodeNum node, NodeNum excludeFrom) const;

    bool pullNext(NodeNum node, ClientBaseLeanRecord &out);
    bool peekNext(NodeNum node, ClientBaseLeanRecord &out) const;

    uint32_t oldestSeq() const { return ring_ ? ring_->oldestSeq() : 0; }

  private:
    struct CursorSlot {
        NodeNum node = 0;
        uint32_t nextSeq = 0;
        bool used = false;
    };

    void freeStorage();
    int findCursor(NodeNum node) const;
    int findFreeCursor() const;
    void clampCursorsToOldest();
    bool copyAtSeq(uint32_t seq, ClientBaseLeanRecord &out) const;

    ClientBaseSecretaryRamRing ramRing_;
    ClientBaseSecretaryRing *ring_ = nullptr;
    CursorSlot *cursors_ = nullptr;
    uint8_t maxFavorites_ = 0;
    uint32_t ttlSec_ = 0;
};
