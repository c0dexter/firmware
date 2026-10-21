// CLIENT_BASE L1 live bridge: never-cancel on configured SECONDARY channels;
// primary stays CLIENT-like unless M2 opt-in elevates primary TEXT.
//
// Pins Channels helpers and ClientBaseRelay.h decisions used by
// FloodingRouter::roleAllowsCancelingDupe / perhapsCancelDupe.
//
// Regression: without secondary protection, cancel-on-dupe + hidden terminal starves
// indoor nodes while a phone on the roof still sees RX (project UC1).

#include "Channels.h"
#include "ClientBaseRelay.h"
#include "TestUtil.h"
#include "configuration.h"
#include "mesh-pb-constants.h"
#include <cstring>
#include <unity.h>

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

void setUp(void)
{
    memset(&channelFile, 0, sizeof(channelFile));
    channels.initDefaults();
    config.device.role = meshtastic_Config_DeviceConfig_Role_CLIENT_BASE;
    clientBaseSetElevatePrimaryOptIn(false);
}

void tearDown(void)
{
    clientBaseSetElevatePrimaryOptIn(false);
}

static void test_primary_is_not_configured_secondary(void)
{
    TEST_ASSERT_FALSE(channels.isConfiguredSecondary(0));
    TEST_ASSERT_TRUE(channels.isConfiguredPrimary(0));
    TEST_ASSERT_EQUAL(meshtastic_Channel_Role_PRIMARY, channels.getByIndex(0).role);
}

static void test_disabled_slot_is_not_configured_secondary(void)
{
    TEST_ASSERT_FALSE(channels.isConfiguredSecondary(3));
}

static void test_secondary_with_settings_is_configured(void)
{
    installSecondaryAt(1, "CHAT");
    TEST_ASSERT_TRUE(channels.isConfiguredSecondary(1));
    TEST_ASSERT_FALSE(channels.isConfiguredSecondary(0));
}

static void test_policy_primary_allows_cancel(void)
{
    TEST_ASSERT_TRUE(clientBaseAllowsCancelingDupe(false, false, false));
    TEST_ASSERT_FALSE(clientBaseUsesLateRebroadcastWindow(false, false, false));
}

static void test_policy_secondary_never_cancels(void)
{
    TEST_ASSERT_FALSE(clientBaseAllowsCancelingDupe(false, true, false));
    TEST_ASSERT_TRUE(clientBaseUsesLateRebroadcastWindow(false, true, false));
}

static void test_policy_favorite_never_cancels_even_on_primary(void)
{
    TEST_ASSERT_FALSE(clientBaseAllowsCancelingDupe(true, false, false));
    TEST_ASSERT_TRUE(clientBaseUsesLateRebroadcastWindow(true, false, false));
}

static void test_integration_secondary_channel_feeds_policy(void)
{
    installSecondaryAt(1, "CHAT");
    const bool sec = channels.isConfiguredSecondary(1);
    const bool pri = channels.isConfiguredSecondary(0);
    TEST_ASSERT_TRUE(sec);
    TEST_ASSERT_FALSE(pri);
    TEST_ASSERT_FALSE(clientBaseAllowsCancelingDupe(false, sec, false));
    TEST_ASSERT_TRUE(clientBaseAllowsCancelingDupe(false, pri, false));
}

static void test_packet_on_secondary_resolves_wire_hash(void)
{
    installSecondaryAt(1, "CHAT");
    const int16_t hash = channels.getHash(1);
    TEST_ASSERT_TRUE(hash >= 0);

    meshtastic_MeshPacket enc = meshtastic_MeshPacket_init_zero;
    enc.which_payload_variant = meshtastic_MeshPacket_encrypted_tag;
    enc.channel = static_cast<uint32_t>(hash);
    TEST_ASSERT_TRUE(channels.isPacketOnConfiguredSecondary(enc));

    meshtastic_MeshPacket decoded = meshtastic_MeshPacket_init_zero;
    decoded.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    decoded.channel = 1;
    TEST_ASSERT_TRUE(channels.isPacketOnConfiguredSecondary(decoded));

    decoded.channel = 0;
    TEST_ASSERT_FALSE(channels.isPacketOnConfiguredSecondary(decoded));
    TEST_ASSERT_TRUE(channels.isPacketOnConfiguredPrimary(decoded));

    enc.channel = 200;
    if (channels.getHash(1) != 200)
        TEST_ASSERT_FALSE(channels.isPacketOnConfiguredSecondary(enc));
}

// M2 / UC2: default opt-in off → primary not elevated.
static void test_m2_default_off_primary_not_elevated(void)
{
    TEST_ASSERT_FALSE(clientBaseGetElevatePrimaryOptIn());
    TEST_ASSERT_FALSE(clientBaseShouldElevatePrimary(false, true, true, meshtastic_PortNum_TEXT_MESSAGE_APP));
    TEST_ASSERT_TRUE(clientBaseAllowsCancelingDupe(false, false, false));
}

// M2 / UC3: opt-in + primary TEXT decoded → elevate.
static void test_m2_opt_in_primary_text_elevates(void)
{
    TEST_ASSERT_TRUE(clientBaseShouldElevatePrimary(true, true, true, meshtastic_PortNum_TEXT_MESSAGE_APP));
    TEST_ASSERT_TRUE(clientBaseShouldElevatePrimary(true, true, true, meshtastic_PortNum_TEXT_MESSAGE_COMPRESSED_APP));
    TEST_ASSERT_FALSE(clientBaseAllowsCancelingDupe(false, false, true));
    TEST_ASSERT_TRUE(clientBaseUsesLateRebroadcastWindow(false, false, true));
}

// M2: opt-in + decoded primary telemetry → do not elevate (TEXT-focused).
static void test_m2_opt_in_primary_telemetry_not_elevated(void)
{
    TEST_ASSERT_FALSE(clientBaseShouldElevatePrimary(true, true, true, meshtastic_PortNum_TELEMETRY_APP));
    TEST_ASSERT_FALSE(clientBaseShouldElevatePrimary(true, true, true, meshtastic_PortNum_POSITION_APP));
}

// M2: opt-in + encrypted primary → elevate (portnum unknown on cancel path).
static void test_m2_opt_in_encrypted_primary_elevates(void)
{
    TEST_ASSERT_TRUE(clientBaseShouldElevatePrimary(true, true, false, meshtastic_PortNum_UNKNOWN_APP));
}

static void test_m2_opt_in_runtime_flag(void)
{
    clientBaseSetElevatePrimaryOptIn(true);
    TEST_ASSERT_TRUE(clientBaseGetElevatePrimaryOptIn());
    clientBaseSetElevatePrimaryOptIn(false);
    TEST_ASSERT_FALSE(clientBaseGetElevatePrimaryOptIn());
}

void setup()
{
    initializeTestEnvironment();
    UNITY_BEGIN();
    RUN_TEST(test_primary_is_not_configured_secondary);
    RUN_TEST(test_disabled_slot_is_not_configured_secondary);
    RUN_TEST(test_secondary_with_settings_is_configured);
    RUN_TEST(test_policy_primary_allows_cancel);
    RUN_TEST(test_policy_secondary_never_cancels);
    RUN_TEST(test_policy_favorite_never_cancels_even_on_primary);
    RUN_TEST(test_integration_secondary_channel_feeds_policy);
    RUN_TEST(test_packet_on_secondary_resolves_wire_hash);
    RUN_TEST(test_m2_default_off_primary_not_elevated);
    RUN_TEST(test_m2_opt_in_primary_text_elevates);
    RUN_TEST(test_m2_opt_in_primary_telemetry_not_elevated);
    RUN_TEST(test_m2_opt_in_encrypted_primary_elevates);
    RUN_TEST(test_m2_opt_in_runtime_flag);
    exit(UNITY_END());
}

void loop() {}
