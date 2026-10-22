#pragma once

#include "ClientBaseSecretaryStore.h"
#include "MeshTypes.h"
#include "mesh-pb-constants.h"
#include "mesh/generated/meshtastic/mesh.pb.h"
#include "mesh/generated/meshtastic/storeforward.pb.h"
#include <cstring>

/**
 * CLIENT_BASE L2 dump wire shape (D1=A).
 * Channel-PSK TEXT unicasts are rejected as legacy DM; match classic StoreForwardModule
 * over-the-air path: STORE_FORWARD ROUTER_TEXT_* with forged from. Zero-hop local dump needs
 * has_bitfield so receivers do not post-decode drop 0/0 as pre-hop firmware.
 * Does not allocate PacketId (caller keeps allocForSending id).
 */
inline void clientBaseSecretaryPrepareDumpPacket(meshtastic_MeshPacket &p, NodeNum dest, const ClientBaseLeanRecord &rec)
{
    p.to = dest;
    p.from = rec.from;
    p.channel = rec.channel;
    p.want_ack = false;
    p.hop_limit = 0;
    p.hop_start = 0;
    p.decoded.has_bitfield = true;
    p.priority = meshtastic_MeshPacket_Priority_BACKGROUND;
    p.which_payload_variant = meshtastic_MeshPacket_decoded_tag;

    meshtastic_StoreAndForward sf = meshtastic_StoreAndForward_init_zero;
    sf.rr = meshtastic_StoreAndForward_RequestResponse_ROUTER_TEXT_BROADCAST;
    sf.which_variant = meshtastic_StoreAndForward_text_tag;
    if (rec.id) {
        sf.original_id = rec.id;
    }
    // S&F text field max is 233, but Data.payload is also 233 - protobuf framing means a
    // full-size text does not encode (pb_encode_to_bytes returns 0). Shrink until it fits.
    const size_t textMax = sizeof(sf.variant.text.bytes);
    size_t n = rec.payload_len > textMax ? textMax : rec.payload_len;
    p.decoded.portnum = meshtastic_PortNum_STORE_FORWARD_APP;
    p.decoded.payload.size = 0;
    while (true) {
        sf.variant.text.size = n;
        if (n) {
            memcpy(sf.variant.text.bytes, rec.payload, n);
        }
        const size_t enc =
            pb_encode_to_bytes(p.decoded.payload.bytes, sizeof(p.decoded.payload.bytes), &meshtastic_StoreAndForward_msg, &sf);
        if (enc) {
            p.decoded.payload.size = enc;
            break;
        }
        if (n == 0) {
            break;
        }
        n--;
    }
}
