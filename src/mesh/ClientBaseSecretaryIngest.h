#pragma once

#include "ClientBaseSecretaryStore.h"
#include "MeshTypes.h"
#include "mesh/generated/meshtastic/mesh.pb.h"
#include <cstring>

/**
 * CLIENT_BASE L2-P1: map decoded TEXT on a configured secondary into the lean store.
 * Pure helpers for Unity; module supplies channel/from-us gates.
 */

/** True when this packet is a candidate for household secretary buffering. */
inline bool clientBaseSecretaryShouldIngest(bool storeReady, bool onConfiguredSecondary, bool fromUs, meshtastic_PortNum portnum)
{
    return storeReady && onConfiguredSecondary && !fromUs && portnum == meshtastic_PortNum_TEXT_MESSAGE_APP;
}

/** Fill lean record from a decoded MeshPacket. Truncates payload to LEAN max. */
inline void clientBaseSecretaryFillLeanRecord(const meshtastic_MeshPacket &mp, uint32_t nowSec, ClientBaseLeanRecord &out)
{
    out = {};
    out.time_sec = nowSec;
    out.from = getFrom(&mp);
    out.id = mp.id;
    out.channel = mp.channel; // post-decode: channel index
    const size_t n = mp.decoded.payload.size;
    out.payload_len = static_cast<uint8_t>(n > CLIENT_BASE_LEAN_PAYLOAD_MAX ? CLIENT_BASE_LEAN_PAYLOAD_MAX : n);
    if (out.payload_len) {
        memcpy(out.payload, mp.decoded.payload.bytes, out.payload_len);
    }
}

/**
 * Expire + push when gates pass. Returns assigned seq (0 = skipped or push fail).
 * Caller must only invoke for packets that already match TEXT + decoded.
 */
inline uint32_t clientBaseSecretaryIngestText(ClientBaseSecretaryStore &store, const meshtastic_MeshPacket &mp, uint32_t nowSec,
                                              bool onConfiguredSecondary, bool fromUs)
{
    if (!clientBaseSecretaryShouldIngest(true, onConfiguredSecondary, fromUs, mp.decoded.portnum)) {
        return 0;
    }
    if (mp.which_payload_variant != meshtastic_MeshPacket_decoded_tag) {
        return 0;
    }

    ClientBaseLeanRecord rec;
    clientBaseSecretaryFillLeanRecord(mp, nowSec, rec);
    store.expire(nowSec);
    return store.push(rec, nowSec);
}
