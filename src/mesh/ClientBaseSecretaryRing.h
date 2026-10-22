#pragma once

#include "MeshTypes.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

/** Lean L2 record size (ANALYSIS §9 / sim_platform_storage.py). */
static constexpr size_t CLIENT_BASE_LEAN_RECORD_BYTES = 256;
static constexpr size_t CLIENT_BASE_LEAN_PAYLOAD_MAX = 236;

struct ClientBaseLeanRecord {
    uint32_t seq = 0;
    uint32_t time_sec = 0;
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
 * L2-P4.1: record ring backend (RAM today; flash+index later on nRF).
 * Favorite cursors stay in ClientBaseSecretaryStore. Not for L1 hot path.
 */
class ClientBaseSecretaryRing
{
  public:
    virtual ~ClientBaseSecretaryRing() = default;

    virtual bool init(uint16_t capacity) = 0;
    virtual void deinit() = 0;
    virtual void clear() = 0;

    virtual uint16_t capacity() const = 0;
    virtual uint16_t size() const = 0;
    virtual uint32_t nextSeq() const = 0;
    virtual uint32_t oldestSeq() const = 0;

    virtual uint32_t push(const ClientBaseLeanRecord &in, uint32_t nowSec, uint32_t ttlSec) = 0;
    virtual void expire(uint32_t nowSec, uint32_t ttlSec) = 0;
    virtual bool copyAtSeq(uint32_t seq, ClientBaseLeanRecord &out) const = 0;
};

class ClientBaseSecretaryRamRing : public ClientBaseSecretaryRing
{
  public:
    ~ClientBaseSecretaryRamRing() override { deinit(); }

    bool init(uint16_t capacity) override
    {
        deinit();
        if (capacity == 0)
            return false;
        ring_ = static_cast<ClientBaseLeanRecord *>(calloc(capacity, sizeof(ClientBaseLeanRecord)));
        if (!ring_)
            return false;
        capacity_ = capacity;
        size_ = 0;
        head_ = 0;
        nextSeq_ = 1;
        return true;
    }

    void deinit() override
    {
        free(ring_);
        ring_ = nullptr;
        capacity_ = 0;
        size_ = 0;
        head_ = 0;
        nextSeq_ = 1;
    }

    void clear() override
    {
        if (!ring_)
            return;
        size_ = 0;
        head_ = 0;
        nextSeq_ = 1;
    }

    uint16_t capacity() const override { return capacity_; }
    uint16_t size() const override { return size_; }
    uint32_t nextSeq() const override { return nextSeq_; }

    uint32_t oldestSeq() const override
    {
        if (!ring_ || size_ == 0)
            return 0;
        return ring_[head_].seq;
    }

    void expire(uint32_t nowSec, uint32_t ttlSec) override
    {
        if (!ring_ || ttlSec == 0)
            return;
        while (size_ > 0) {
            const ClientBaseLeanRecord &oldest = ring_[head_];
            if (nowSec < oldest.time_sec)
                break;
            if ((nowSec - oldest.time_sec) < ttlSec)
                break;
            dropOldest();
        }
    }

    uint32_t push(const ClientBaseLeanRecord &in, uint32_t nowSec, uint32_t ttlSec) override
    {
        if (!ring_)
            return 0;
        expire(nowSec, ttlSec);
        if (size_ == capacity_)
            dropOldest();
        const uint16_t tail = static_cast<uint16_t>((head_ + size_) % capacity_);
        ClientBaseLeanRecord &slot = ring_[tail];
        slot = in;
        slot.seq = nextSeq_++;
        if (slot.payload_len > CLIENT_BASE_LEAN_PAYLOAD_MAX)
            slot.payload_len = CLIENT_BASE_LEAN_PAYLOAD_MAX;
        if (slot.time_sec == 0)
            slot.time_sec = nowSec;
        size_++;
        return slot.seq;
    }

    bool copyAtSeq(uint32_t seq, ClientBaseLeanRecord &out) const override
    {
        const uint16_t idx = indexOfSeq(seq);
        if (idx >= capacity_)
            return false;
        out = ring_[idx];
        return true;
    }

  private:
    void dropOldest()
    {
        if (!ring_ || size_ == 0)
            return;
        head_ = static_cast<uint16_t>((head_ + 1) % capacity_);
        size_--;
    }

    uint16_t indexOfSeq(uint32_t seq) const
    {
        if (!ring_ || size_ == 0 || seq == 0)
            return capacity_;
        const uint32_t oldest = ring_[head_].seq;
        if (seq < oldest || seq >= oldest + size_)
            return capacity_;
        const uint16_t offset = static_cast<uint16_t>(seq - oldest);
        return static_cast<uint16_t>((head_ + offset) % capacity_);
    }

    ClientBaseLeanRecord *ring_ = nullptr;
    uint16_t capacity_ = 0;
    uint16_t size_ = 0;
    uint16_t head_ = 0;
    uint32_t nextSeq_ = 1;
};
