#include "ClientBaseSecretaryStore.h"

#include <cstdlib>

void ClientBaseSecretaryStore::freeStorage()
{
    free(ring_);
    free(cursors_);
    ring_ = nullptr;
    cursors_ = nullptr;
    capacity_ = 0;
    size_ = 0;
    head_ = 0;
    maxFavorites_ = 0;
    ttlSec_ = 0;
    nextSeq_ = 1;
}

bool ClientBaseSecretaryStore::init(uint16_t capacity, uint8_t maxFavorites, uint32_t ttlSec)
{
    freeStorage();
    if (capacity == 0 || maxFavorites == 0)
        return false;

    ring_ = static_cast<ClientBaseLeanRecord *>(calloc(capacity, sizeof(ClientBaseLeanRecord)));
    cursors_ = static_cast<CursorSlot *>(calloc(maxFavorites, sizeof(CursorSlot)));
    if (!ring_ || !cursors_) {
        freeStorage();
        return false;
    }

    capacity_ = capacity;
    maxFavorites_ = maxFavorites;
    ttlSec_ = ttlSec;
    size_ = 0;
    head_ = 0;
    nextSeq_ = 1;
    return true;
}

void ClientBaseSecretaryStore::clear()
{
    if (!ring_)
        return;
    size_ = 0;
    head_ = 0;
    nextSeq_ = 1;
    for (uint8_t i = 0; i < maxFavorites_; i++) {
        if (cursors_[i].used)
            cursors_[i].nextSeq = 1;
    }
}

int ClientBaseSecretaryStore::findCursor(NodeNum node) const
{
    if (!cursors_)
        return -1;
    for (uint8_t i = 0; i < maxFavorites_; i++) {
        if (cursors_[i].used && cursors_[i].node == node)
            return i;
    }
    return -1;
}

int ClientBaseSecretaryStore::findFreeCursor() const
{
    if (!cursors_)
        return -1;
    for (uint8_t i = 0; i < maxFavorites_; i++) {
        if (!cursors_[i].used)
            return i;
    }
    return -1;
}

bool ClientBaseSecretaryStore::ensureFavorite(NodeNum node)
{
    if (node == 0 || node == NODENUM_BROADCAST)
        return false;
    if (findCursor(node) >= 0)
        return true;
    const int slot = findFreeCursor();
    if (slot < 0)
        return false;
    cursors_[slot].used = true;
    cursors_[slot].node = node;
    cursors_[slot].nextSeq = oldestSeq() ? oldestSeq() : nextSeq_;
    return true;
}

bool ClientBaseSecretaryStore::hasFavorite(NodeNum node) const
{
    return findCursor(node) >= 0;
}

uint32_t ClientBaseSecretaryStore::oldestSeq() const
{
    if (!ring_ || size_ == 0)
        return 0;
    return ring_[head_].seq;
}

uint16_t ClientBaseSecretaryStore::indexOfSeq(uint32_t seq) const
{
    if (!ring_ || size_ == 0 || seq == 0)
        return capacity_; // invalid sentinel
    const uint32_t oldest = ring_[head_].seq;
    if (seq < oldest || seq >= oldest + size_)
        return capacity_;
    const uint16_t offset = static_cast<uint16_t>(seq - oldest);
    return static_cast<uint16_t>((head_ + offset) % capacity_);
}

void ClientBaseSecretaryStore::clampCursorsToOldest()
{
    const uint32_t oldest = oldestSeq();
    if (oldest == 0)
        return;
    for (uint8_t i = 0; i < maxFavorites_; i++) {
        if (cursors_[i].used && cursors_[i].nextSeq < oldest)
            cursors_[i].nextSeq = oldest;
    }
}

void ClientBaseSecretaryStore::dropOldest()
{
    if (!ring_ || size_ == 0)
        return;
    head_ = static_cast<uint16_t>((head_ + 1) % capacity_);
    size_--;
    clampCursorsToOldest();
}

uint32_t ClientBaseSecretaryStore::push(const ClientBaseLeanRecord &in, uint32_t nowSec)
{
    if (!ring_)
        return 0;

    expire(nowSec);

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

void ClientBaseSecretaryStore::expire(uint32_t nowSec)
{
    if (!ring_ || ttlSec_ == 0)
        return;
    while (size_ > 0) {
        const ClientBaseLeanRecord &oldest = ring_[head_];
        if (nowSec < oldest.time_sec)
            break; // clock went backwards / unset - stop
        if ((nowSec - oldest.time_sec) < ttlSec_)
            break;
        dropOldest();
    }
}

uint16_t ClientBaseSecretaryStore::availableCount(NodeNum node) const
{
    const int ci = findCursor(node);
    if (ci < 0 || size_ == 0)
        return 0;
    const uint32_t oldest = ring_[head_].seq;
    const uint32_t newest = oldest + size_ - 1;
    uint32_t cursor = cursors_[ci].nextSeq;
    if (cursor < oldest)
        cursor = oldest;
    if (cursor > newest + 1)
        return 0;
    if (cursor > newest)
        return 0;
    return static_cast<uint16_t>(newest - cursor + 1);
}

bool ClientBaseSecretaryStore::copyAtSeq(uint32_t seq, ClientBaseLeanRecord &out) const
{
    const uint16_t idx = indexOfSeq(seq);
    if (idx >= capacity_)
        return false;
    out = ring_[idx];
    return true;
}

bool ClientBaseSecretaryStore::peekNext(NodeNum node, ClientBaseLeanRecord &out) const
{
    const int ci = findCursor(node);
    if (ci < 0 || size_ == 0)
        return false;
    uint32_t cursor = cursors_[ci].nextSeq;
    const uint32_t oldest = ring_[head_].seq;
    if (cursor < oldest)
        cursor = oldest;
    return copyAtSeq(cursor, out);
}

bool ClientBaseSecretaryStore::pullNext(NodeNum node, ClientBaseLeanRecord &out)
{
    const int ci = findCursor(node);
    if (ci < 0 || size_ == 0)
        return false;
    uint32_t cursor = cursors_[ci].nextSeq;
    const uint32_t oldest = ring_[head_].seq;
    if (cursor < oldest)
        cursor = oldest;
    if (!copyAtSeq(cursor, out))
        return false;
    cursors_[ci].nextSeq = cursor + 1;
    return true;
}
