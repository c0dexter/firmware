#pragma once

#include "MeshTypes.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

/** Lean L2 record size (ANALYSIS §9 / sim_platform_storage.py). */
static constexpr size_t CLIENT_BASE_LEAN_RECORD_BYTES = 256;
static constexpr size_t CLIENT_BASE_LEAN_PAYLOAD_MAX = 236;

struct ClientBaseLeanRecord {
    uint32_t seq = 0;      // monotonic id assigned on push
    uint32_t time_sec = 0; // capture time (caller-supplied; testable)
    uint32_t from = 0;
    uint32_t id = 0;
    uint8_t channel = 0;
    uint8_t payload_len = 0;
    uint8_t flags = 0;
    uint8_t _pad = 0;
    uint8_t payload[CLIENT_BASE_LEAN_PAYLOAD_MAX]{};
};
static_assert(sizeof(ClientBaseLeanRecord) == CLIENT_BASE_LEAN_RECORD_BYTES, "lean record must stay 256 B");

/**
 * Shared ring + per-favorite cursor store for CLIENT_BASE L2.
 * Advancing one client's cursor never deletes the ring entry (no purge-on-ACK).
 * RAM-backed; flash spill is a later backend.
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

    uint16_t capacity() const { return capacity_; }
    uint16_t size() const { return size_; }
    uint8_t maxFavorites() const { return maxFavorites_; }
    uint32_t ttlSec() const { return ttlSec_; }
    uint32_t nextSeq() const { return nextSeq_; }

    /** Push TEXT-like lean record; assigns seq. Drops oldest if full. Returns assigned seq (0 on fail). */
    uint32_t push(const ClientBaseLeanRecord &in, uint32_t nowSec);

    /** Drop records older than ttl relative to nowSec; bump cursors past expired. */
    void expire(uint32_t nowSec);

    /** Register a served favorite cursor slot (ACL). False if full or already at cap. */
    bool ensureFavorite(NodeNum node);

    bool hasFavorite(NodeNum node) const;

    /** Messages available for this favorite (cursor → live tail). 0 if unknown node. */
    uint16_t availableCount(NodeNum node) const;

    /** Like availableCount but skip records whose from == excludeFrom (requester's own TEXTs). */
    uint16_t availableCountNotFrom(NodeNum node, NodeNum excludeFrom) const;

    /**
     * Copy next record for node into out and advance that cursor only.
     * Returns false if none available or node not registered.
     */
    bool pullNext(NodeNum node, ClientBaseLeanRecord &out);

    /** Peek without advancing. */
    bool peekNext(NodeNum node, ClientBaseLeanRecord &out) const;

    /** Lowest live seq in the ring (0 if empty). */
    uint32_t oldestSeq() const;

  private:
    struct CursorSlot {
        NodeNum node = 0;
        uint32_t nextSeq = 0; // first undelivered seq
        bool used = false;
    };

    void freeStorage();
    int findCursor(NodeNum node) const;
    int findFreeCursor() const;
    void dropOldest();
    void clampCursorsToOldest();
    bool copyAtSeq(uint32_t seq, ClientBaseLeanRecord &out) const;
    uint16_t indexOfSeq(uint32_t seq) const;

    ClientBaseLeanRecord *ring_ = nullptr;
    CursorSlot *cursors_ = nullptr;
    uint16_t capacity_ = 0;
    uint16_t size_ = 0;
    uint16_t head_ = 0; // index of oldest
    uint8_t maxFavorites_ = 0;
    uint32_t ttlSec_ = 0;
    uint32_t nextSeq_ = 1; // seq 0 reserved = invalid
};
