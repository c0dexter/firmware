#pragma once

#include <cstdint>

/**
 * CLIENT_BASE L1.5 hop policy (#8366 / korbinianbauer design).
 *
 * - FreeHopBumpStart: heard directly from a favorite (hopsAway==0) → keep hop_limit, hop_start++.
 *   Accurate travel count; next CLIENT_BASE won't treat it as another "direct from fav".
 * - DeliverAtZero: hop_limit already 0 but packet is to/from a favorite → still TX with hop_limit=0
 *   (last meter into the house) without inventing remaining budget for the public mesh.
 * - NormalDecrement: everyone else / non-CLIENT_BASE.
 */

enum class ClientBaseRelayHop : uint8_t {
    NormalDecrement = 0,
    FreeHopBumpStart = 1,
    DeliverAtZero = 2,
};

inline ClientBaseRelayHop clientBaseClassifyRelayHop(bool localIsClientBase, bool fromFavorite, bool toFavorite, int8_t hopsAway,
                                                     uint8_t hopLimit)
{
    if (!localIsClientBase) {
        return ClientBaseRelayHop::NormalDecrement;
    }

    // Direct RF from a served favorite (originator TX: hop_start == hop_limit).
    if (fromFavorite && hopsAway == 0) {
        return ClientBaseRelayHop::FreeHopBumpStart;
    }

    // Last-meter: budget exhausted, but household still needs the roof→indoor hop.
    if (hopLimit == 0 && (fromFavorite || toFavorite)) {
        return ClientBaseRelayHop::DeliverAtZero;
    }

    return ClientBaseRelayHop::NormalDecrement;
}
