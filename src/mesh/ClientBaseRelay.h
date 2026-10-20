#pragma once

#include "MeshTypes.h"

/**
 * CLIENT_BASE L1: return true if we may cancel a queued rebroadcast after hearing a dupe.
 * Protected (never cancel) when from/to is a served favorite OR the packet is on a
 * configured secondary channel held by this base.
 */
inline bool clientBaseAllowsCancelingDupe(bool fromOrToFavorite, bool onConfiguredSecondary)
{
    return !(fromOrToFavorite || onConfiguredSecondary);
}

/** True when CLIENT_BASE should use ROUTER_LATE-style late rebroadcast window for this packet. */
inline bool clientBaseUsesLateRebroadcastWindow(bool fromOrToFavorite, bool onConfiguredSecondary)
{
    return fromOrToFavorite || onConfiguredSecondary;
}
