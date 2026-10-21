// CLIENT_BASE L2-P1 ingest: secondary TEXT → lean store; primary / own TX skipped.
//
// Pins ClientBaseSecretaryIngest.h gates used by ClientBaseSecretaryModule::handleReceived.
// Does not cover poll/unicast (P2) or MeshModule wiring.

#include "Channels.h"
#include "ClientBaseSecretaryIngest.h"
#include "ClientBaseSecretaryStore.h"
#include "TestUtil.h"
#include "configuration.h"
#include "mesh-pb-constants.h"
#include <cstring>
#include <unity.h>

static ClientBaseSecretaryStore *store = nullptr;

static void installSecondaryAt(ChannelIndex idx, const char *name)
{
    meshtastic_Channel ch = meshtastic_Channel_init_zero;
    ch.index = idx;
    ch.has_settings = true;
    ch.role = meshtastic_Channel_Role_SECONDARY;
    strncpy(ch.settings.name, name, sizeof(ch.settings.name) - 1);
    ch.settings.psk.size = 1;
    ch.settings.psk.bytes[0] = 2;
    channels.setChannel(ch);
    channels.fixupChannel(idx);
}

static meshtastic_MeshPacket makeText(NodeNum from, PacketId id, uint8_t channelIndex, const char *text)
{
    meshtastic_MeshPacket mp = meshtastic_MeshPacket_init_zero;
    mp.from = from;
    mp.id = id;
    mp.channel = channelIndex;
    mp.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    mp.decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    const size_t n = text ? strlen(text) : 0;
    mp.decoded.payload.size = n > sizeof(mp.decoded.payload.bytes) ? sizeof(mp.decoded.payload.bytes) : n;
    if (n) {
        memcpy(mp.decoded.payload.bytes, text, mp.decoded.payload.size);
    }
    return mp;
}

void setUp(void)
{
    memset(&channelFile, 0, sizeof(channelFile));
    channels.initDefaults();
    config.device.role = meshtastic_Config_DeviceConfig_Role_CLIENT_BASE;
    store = new ClientBaseSecretaryStore();
    TEST_ASSERT_TRUE(store->init(16, 4, 3600));
}

void tearDown(void)
{
    delete store;
    store = nullptr;
}

static void test_should_ingest_gates(void)
{
    TEST_ASSERT_TRUE(clientBaseSecretaryShouldIngest(true, true, false, meshtastic_PortNum_TEXT_MESSAGE_APP));
    TEST_ASSERT_FALSE(clientBaseSecretaryShouldIngest(false, true, false, meshtastic_PortNum_TEXT_MESSAGE_APP));
    TEST_ASSERT_FALSE(clientBaseSecretaryShouldIngest(true, false, false, meshtastic_PortNum_TEXT_MESSAGE_APP));
    TEST_ASSERT_FALSE(clientBaseSecretaryShouldIngest(true, true, true, meshtastic_PortNum_TEXT_MESSAGE_APP));
    TEST_ASSERT_FALSE(clientBaseSecretaryShouldIngest(true, true, false, meshtastic_PortNum_POSITION_APP));
}

static void test_ingest_secondary_text_available_to_favorite(void)
{
    installSecondaryAt(1, "home");
    TEST_ASSERT_TRUE(channels.isConfiguredSecondary(1));
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));

    meshtastic_MeshPacket mp = makeText(0x10, 42, 1, "hello-home");
    TEST_ASSERT_TRUE(channels.isPacketOnConfiguredSecondary(mp));

    const uint32_t seq = clientBaseSecretaryIngestText(*store, mp, 100, true, false);
    TEST_ASSERT_EQUAL_UINT32(1, seq);
    TEST_ASSERT_EQUAL_UINT16(1, store->availableCount(0xA1));

    ClientBaseLeanRecord out{};
    TEST_ASSERT_TRUE(store->pullNext(0xA1, out));
    TEST_ASSERT_EQUAL_UINT32(0x10, out.from);
    TEST_ASSERT_EQUAL_UINT32(42, out.id);
    TEST_ASSERT_EQUAL_UINT8(1, out.channel);
    TEST_ASSERT_EQUAL_STRING_LEN("hello-home", (const char *)out.payload, out.payload_len);
}

static void test_ingest_skips_primary(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    meshtastic_MeshPacket mp = makeText(0x10, 1, 0, "longfast");
    TEST_ASSERT_FALSE(channels.isConfiguredSecondary(0));

    TEST_ASSERT_EQUAL_UINT32(0, clientBaseSecretaryIngestText(*store, mp, 100, false, false));
    TEST_ASSERT_EQUAL_UINT16(0, store->size());
    TEST_ASSERT_EQUAL_UINT16(0, store->availableCount(0xA1));
}

static void test_ingest_skips_own_tx(void)
{
    installSecondaryAt(1, "home");
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    meshtastic_MeshPacket mp = makeText(0x10, 2, 1, "mine");

    TEST_ASSERT_EQUAL_UINT32(0, clientBaseSecretaryIngestText(*store, mp, 100, true, true));
    TEST_ASSERT_EQUAL_UINT16(0, store->size());
}

static void test_ingest_truncates_oversize_payload(void)
{
    installSecondaryAt(1, "home");
    char big[300];
    memset(big, 'x', sizeof(big) - 1);
    big[sizeof(big) - 1] = 0;

    meshtastic_MeshPacket mp = makeText(0x10, 3, 1, big);
    // makeText already capped to DATA_PAYLOAD_LEN; force oversize for lean max test via fill helper
    ClientBaseLeanRecord rec{};
    mp.decoded.payload.size = CLIENT_BASE_LEAN_PAYLOAD_MAX + 20;
    memset(mp.decoded.payload.bytes, 'y', sizeof(mp.decoded.payload.bytes));
    clientBaseSecretaryFillLeanRecord(mp, 50, rec);
    TEST_ASSERT_EQUAL_UINT8(CLIENT_BASE_LEAN_PAYLOAD_MAX, rec.payload_len);
}

void setup()
{
    initializeTestEnvironment();
    UNITY_BEGIN();
    RUN_TEST(test_should_ingest_gates);
    RUN_TEST(test_ingest_secondary_text_available_to_favorite);
    RUN_TEST(test_ingest_skips_primary);
    RUN_TEST(test_ingest_skips_own_tx);
    RUN_TEST(test_ingest_truncates_oversize_payload);
    exit(UNITY_END());
}

void loop() {}
