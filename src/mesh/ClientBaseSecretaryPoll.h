#pragma once

#include "ClientBaseSecretaryStore.h"
#include "MeshTypes.h"
#include "mesh/ClientBaseSecretaryPolicy.h"
#include <cstdint>

/**
 * CLIENT_BASE L2-P2 poll session (D1=A control plane).
 * Pure helpers for Unity; air TX stays in the module.
 */

enum class ClientBaseSecretaryPollStart : uint8_t {
    Rejected = 0, // not favorite / store full / not ready
    Busy = 1,     // another dump in progress
    Empty = 2,    // ACL ok, nothing buffered for this cursor
    Started = 3,  // session armed; pull frames until done
};

struct ClientBaseSecretaryPollSession {
    bool busy = false;
    NodeNum busyTo = 0;
    uint16_t remaining = 0;
    uint16_t sent = 0;
};

inline void clientBaseSecretaryPollClear(ClientBaseSecretaryPollSession &session)
{
    session.busy = false;
    session.busyTo = 0;
    session.remaining = 0;
    session.sent = 0;
}

/**
 * Begin a poll for requester. Registers cursor slot if needed.
 * historyReturnMax caps frames this session (D2).
 */
inline ClientBaseSecretaryPollStart clientBaseSecretaryBeginPoll(ClientBaseSecretaryStore &store,
                                                                 ClientBaseSecretaryPollSession &session, NodeNum requester,
                                                                 bool isFavorite, uint16_t historyReturnMax)
{
    if (!isFavorite || requester == 0) {
        return ClientBaseSecretaryPollStart::Rejected;
    }
    if (session.busy) {
        return ClientBaseSecretaryPollStart::Busy;
    }
    if (!store.hasFavorite(requester) && !store.ensureFavorite(requester)) {
        return ClientBaseSecretaryPollStart::Rejected;
    }

    // Exclude requester's own TEXTs (classic S&F behavior; avoids from==to dump noise).
    uint16_t avail = store.availableCountNotFrom(requester, requester);
    if (historyReturnMax == 0) {
        historyReturnMax = CLIENT_BASE_SECRETARY_HISTORY_RETURN_MAX;
    }
    const uint16_t n = avail > historyReturnMax ? historyReturnMax : avail;
    if (n == 0) {
        return ClientBaseSecretaryPollStart::Empty;
    }

    session.busy = true;
    session.busyTo = requester;
    session.remaining = n;
    session.sent = 0;
    return ClientBaseSecretaryPollStart::Started;
}

/** Pull next lean record for the active session; clears busy when exhausted. */
inline bool clientBaseSecretaryPollPullNext(ClientBaseSecretaryStore &store, ClientBaseSecretaryPollSession &session,
                                            ClientBaseLeanRecord &out)
{
    if (!session.busy || session.remaining == 0) {
        return false;
    }
    if (!store.pullNext(session.busyTo, out)) {
        clientBaseSecretaryPollClear(session);
        return false;
    }
    session.remaining--;
    session.sent++;
    if (session.remaining == 0) {
        session.busy = false;
        // keep busyTo/sent for logging until next begin
    }
    return true;
}

/** Abort only if requester owns the active dump (CLIENT_ABORT). */
inline bool clientBaseSecretaryPollAbort(ClientBaseSecretaryPollSession &session, NodeNum requester)
{
    if (session.busy && session.busyTo == requester) {
        clientBaseSecretaryPollClear(session);
        return true;
    }
    return false;
}
