#include "ClientBaseSecretaryStore.h"

#include <cstdlib>

void ClientBaseSecretaryStore::freeStorage()
{
    if (ring_) {
        ring_->deinit();
        ring_ = nullptr;
    }
    free(cursors_);
    cursors_ = nullptr;
    maxFavorites_ = 0;
    ttlSec_ = 0;
}

bool ClientBaseSecretaryStore::init(uint16_t capacity, uint8_t maxFavorites, uint32_t ttlSec)
{
    freeStorage();
    if (capacity == 0 || maxFavorites == 0)
        return false;

    if (!ramRing_.init(capacity)) {
        freeStorage();
        return false;
    }
    ring_ = &ramRing_;

    cursors_ = static_cast<CursorSlot *>(calloc(maxFavorites, sizeof(CursorSlot)));
    if (!cursors_) {
        freeStorage();
        return false;
    }

    maxFavorites_ = maxFavorites;
    ttlSec_ = ttlSec;
    return true;
}

void ClientBaseSecretaryStore::clear()
{
    if (!ring_)
        return;
    ring_->clear();
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
    cursors_[slot].nextSeq = oldestSeq() ? oldestSeq() : nextSeq();
    return true;
}

bool ClientBaseSecretaryStore::hasFavorite(NodeNum node) const
{
    return findCursor(node) >= 0;
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

uint32_t ClientBaseSecretaryStore::push(const ClientBaseLeanRecord &in, uint32_t nowSec)
{
    if (!ring_)
        return 0;
    const uint32_t seq = ring_->push(in, nowSec, ttlSec_);
    if (seq)
        clampCursorsToOldest();
    return seq;
}

void ClientBaseSecretaryStore::expire(uint32_t nowSec)
{
    if (!ring_)
        return;
    ring_->expire(nowSec, ttlSec_);
    clampCursorsToOldest();
}

uint16_t ClientBaseSecretaryStore::availableCount(NodeNum node) const
{
    const int ci = findCursor(node);
    if (ci < 0 || !ring_ || ring_->size() == 0)
        return 0;
    const uint32_t oldest = ring_->oldestSeq();
    const uint32_t newest = oldest + ring_->size() - 1;
    uint32_t cursor = cursors_[ci].nextSeq;
    if (cursor < oldest)
        cursor = oldest;
    if (cursor > newest)
        return 0;
    return static_cast<uint16_t>(newest - cursor + 1);
}

uint16_t ClientBaseSecretaryStore::availableCountNotFrom(NodeNum node, NodeNum excludeFrom) const
{
    const int ci = findCursor(node);
    if (ci < 0 || !ring_ || ring_->size() == 0)
        return 0;
    const uint32_t oldest = ring_->oldestSeq();
    const uint32_t newest = oldest + ring_->size() - 1;
    uint32_t cursor = cursors_[ci].nextSeq;
    if (cursor < oldest)
        cursor = oldest;
    if (cursor > newest)
        return 0;
    uint16_t n = 0;
    for (uint32_t seq = cursor; seq <= newest; seq++) {
        ClientBaseLeanRecord rec;
        if (!copyAtSeq(seq, rec))
            break;
        if (rec.from != excludeFrom)
            n++;
    }
    return n;
}

bool ClientBaseSecretaryStore::copyAtSeq(uint32_t seq, ClientBaseLeanRecord &out) const
{
    return ring_ && ring_->copyAtSeq(seq, out);
}

bool ClientBaseSecretaryStore::peekNext(NodeNum node, ClientBaseLeanRecord &out) const
{
    const int ci = findCursor(node);
    if (ci < 0 || !ring_ || ring_->size() == 0)
        return false;
    uint32_t cursor = cursors_[ci].nextSeq;
    const uint32_t oldest = ring_->oldestSeq();
    if (cursor < oldest)
        cursor = oldest;
    return copyAtSeq(cursor, out);
}

bool ClientBaseSecretaryStore::pullNext(NodeNum node, ClientBaseLeanRecord &out)
{
    const int ci = findCursor(node);
    if (ci < 0 || !ring_ || ring_->size() == 0)
        return false;
    uint32_t cursor = cursors_[ci].nextSeq;
    const uint32_t oldest = ring_->oldestSeq();
    if (cursor < oldest)
        cursor = oldest;
    if (!copyAtSeq(cursor, out))
        return false;
    cursors_[ci].nextSeq = cursor + 1;
    return true;
}
