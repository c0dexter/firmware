#pragma once

#include "MeshTypes.h"
#include "mesh/generated/meshtastic/mesh.pb.h"

/**
 * CLIENT_BASE L1 live-bridge helpers (testable, no flash I/O).
 *
 * Protected (never cancel / late window) when:
 * - from/to is a served favorite, OR
 * - packet is on a configured secondary, OR
 * - M2 opt-in elevates this primary packet (see clientBaseShouldElevatePrimary).
 */

/** M2: opt-in primary elevate. Encrypted primary must elevate (portnum unknown pre-decode). */
inline bool clientBaseShouldElevatePrimary(bool optInEnabled, bool onConfiguredPrimary, bool isDecoded,
                                           meshtastic_PortNum portnum)
{
    if (!optInEnabled || !onConfiguredPrimary) {
        return false;
    }
    if (!isDecoded) {
        return true;
    }
    return portnum == meshtastic_PortNum_TEXT_MESSAGE_APP || portnum == meshtastic_PortNum_TEXT_MESSAGE_COMPRESSED_APP;
}

inline bool clientBaseAllowsCancelingDupe(bool fromOrToFavorite, bool onConfiguredSecondary, bool elevatePrimary)
{
    return !(fromOrToFavorite || onConfiguredSecondary || elevatePrimary);
}

inline bool clientBaseUsesLateRebroadcastWindow(bool fromOrToFavorite, bool onConfiguredSecondary, bool elevatePrimary)
{
    return fromOrToFavorite || onConfiguredSecondary || elevatePrimary;
}

/**
 * Runtime M2 flag (default off). Full protobuf config deferred - needs protobufs PR.
 * Tests / future admin path call setter; firmware defaults false (UC2 honest).
 */
bool clientBaseGetElevatePrimaryOptIn();
void clientBaseSetElevatePrimaryOptIn(bool enabled);
