// CLIENT_BASE L2 dump wire: STORE_FORWARD ROUTER_TEXT_* + zero-hop bitfield.
//
// Pins ClientBaseSecretaryDump.h used by ClientBaseSecretaryModule::sendNextPayload.
// Guards desk regressions: channel-PSK TEXT DM (legacy DM reject) and 0/0 without
// has_bitfield (post-decode pre-hop drop). Does not exercise LoRa TX.

#include "ClientBaseSecretaryDump.h"
#include "ClientBaseSecretaryStore.h"
#include "NodeDB.h"
#include "TestUtil.h"
#include "mesh-pb-constants.h"
#include "mesh/generated/meshtastic/storeforward.pb.h"
#include <cstring>
#include <unity.h>

static ClientBaseLeanRecord makeRec(uint32_t from, uint32_t id, uint8_t channel, const char *text)
{
    ClientBaseLeanRecord r{};
    r.from = from;
    r.id = id;
    r.channel = channel;
    if (text) {
        const size_t n = strlen(text);
        r.payload_len = static_cast<uint8_t>(n > CLIENT_BASE_LEAN_PAYLOAD_MAX ? CLIENT_BASE_LEAN_PAYLOAD_MAX : n);
        memcpy(r.payload, text, r.payload_len);
    }
    return r;
}

static void test_dump_is_store_forward_router_text_not_plain_text(void)
{
    meshtastic_MeshPacket p = meshtastic_MeshPacket_init_zero;
    p.id = 0xAABBCCDDu; // caller-owned PacketId
    const ClientBaseLeanRecord rec = makeRec(0x2cdd1088u, 0x11112222u, 1, "desk-seed");

    clientBaseSecretaryPrepareDumpPacket(p, 0xebba7ec6u, rec);

    TEST_ASSERT_EQUAL_UINT32(0xAABBCCDDu, p.id);
    TEST_ASSERT_EQUAL_UINT32(0x2cdd1088u, p.from);
    TEST_ASSERT_EQUAL_UINT32(0xebba7ec6u, p.to);
    TEST_ASSERT_EQUAL_UINT8(1, p.channel);
    TEST_ASSERT_EQUAL(meshtastic_PortNum_STORE_FORWARD_APP, p.decoded.portnum);
    TEST_ASSERT_FALSE(p.want_ack);
    TEST_ASSERT_EQUAL(meshtastic_MeshPacket_Priority_BACKGROUND, p.priority);

    meshtastic_StoreAndForward sf = meshtastic_StoreAndForward_init_zero;
    TEST_ASSERT_TRUE(pb_decode_from_bytes(p.decoded.payload.bytes, p.decoded.payload.size, &meshtastic_StoreAndForward_msg, &sf));
    TEST_ASSERT_EQUAL(meshtastic_StoreAndForward_RequestResponse_ROUTER_TEXT_BROADCAST, sf.rr);
    TEST_ASSERT_EQUAL(meshtastic_StoreAndForward_text_tag, sf.which_variant);
    TEST_ASSERT_EQUAL_UINT32(0x11112222u, sf.original_id);
    TEST_ASSERT_EQUAL_UINT32(strlen("desk-seed"), sf.variant.text.size);
    TEST_ASSERT_EQUAL_INT(0, memcmp(sf.variant.text.bytes, "desk-seed", sf.variant.text.size));
}

static void test_dump_zero_hop_has_bitfield_so_prehop_accepts(void)
{
    meshtastic_MeshPacket p = meshtastic_MeshPacket_init_zero;
    p.id = 1;
    clientBaseSecretaryPrepareDumpPacket(p, 0xA1, makeRec(0xB2, 9, 1, "x"));

    TEST_ASSERT_EQUAL_UINT8(0, p.hop_limit);
    TEST_ASSERT_EQUAL_UINT8(0, p.hop_start);
    TEST_ASSERT_TRUE(p.decoded.has_bitfield);
    // Same predicate receivers use post-decode; without bitfield this is MISSING_OR_UNKNOWN → drop.
    TEST_ASSERT_EQUAL((int)HopStartStatus::VALID, (int)classifyHopStart(p));
}

static void test_dump_shrinks_text_until_sf_fits_data_payload(void)
{
    // Lean max 236 and S&F text field 233 both exceed what fits once SF is framed into
    // Data.payload (also 233). Encode must succeed with a truncated prefix, not size 0.
    ClientBaseLeanRecord rec{};
    rec.from = 1;
    rec.id = 2;
    rec.channel = 1;
    rec.payload_len = CLIENT_BASE_LEAN_PAYLOAD_MAX;
    memset(rec.payload, 'A', rec.payload_len);

    meshtastic_MeshPacket p = meshtastic_MeshPacket_init_zero;
    p.id = 3;
    clientBaseSecretaryPrepareDumpPacket(p, 0xA1, rec);

    TEST_ASSERT_TRUE(p.decoded.payload.size > 0);
    TEST_ASSERT_TRUE(p.decoded.payload.size <= meshtastic_Constants_DATA_PAYLOAD_LEN);

    meshtastic_StoreAndForward sf = meshtastic_StoreAndForward_init_zero;
    TEST_ASSERT_TRUE(pb_decode_from_bytes(p.decoded.payload.bytes, p.decoded.payload.size, &meshtastic_StoreAndForward_msg, &sf));
    TEST_ASSERT_TRUE(sf.variant.text.size > 0);
    TEST_ASSERT_TRUE(sf.variant.text.size < rec.payload_len);
    TEST_ASSERT_TRUE(sf.variant.text.size < sizeof(sf.variant.text.bytes));
    TEST_ASSERT_EQUAL_INT(0, memcmp(sf.variant.text.bytes, rec.payload, sf.variant.text.size));
}

void setup()
{
    initializeTestEnvironment();
    UNITY_BEGIN();
    RUN_TEST(test_dump_is_store_forward_router_text_not_plain_text);
    RUN_TEST(test_dump_zero_hop_has_bitfield_so_prehop_accepts);
    RUN_TEST(test_dump_shrinks_text_until_sf_fits_data_payload);
    exit(UNITY_END());
}

void loop() {}
