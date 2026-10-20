// CLIENT_BASE L1 live bridge: never-cancel on configured SECONDARY channels;
// primary stays CLIENT-like (cancel allowed) unless a later opt-in elevates TEXT.
//
// Pins Channels::isConfiguredSecondary and ClientBaseRelay.h decision helpers used by
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
}

void setUp(void)
{
    memset(&channelFile, 0, sizeof(channelFile));
    channels.initDefaults();
    config.device.role = meshtastic_Config_DeviceConfig_Role_CLIENT_BASE;
}

void tearDown(void) {}

static void test_primary_is_not_configured_secondary(void)
{
    TEST_ASSERT_FALSE(channels.isConfiguredSecondary(0));
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
    TEST_ASSERT_TRUE(clientBaseAllowsCancelingDupe(/*fav=*/false, /*secondary=*/false));
    TEST_ASSERT_FALSE(clientBaseUsesLateRebroadcastWindow(false, false));
}

static void test_policy_secondary_never_cancels(void)
{
    TEST_ASSERT_FALSE(clientBaseAllowsCancelingDupe(false, true));
    TEST_ASSERT_TRUE(clientBaseUsesLateRebroadcastWindow(false, true));
}

static void test_policy_favorite_never_cancels_even_on_primary(void)
{
    TEST_ASSERT_FALSE(clientBaseAllowsCancelingDupe(true, false));
    TEST_ASSERT_TRUE(clientBaseUsesLateRebroadcastWindow(true, false));
}

static void test_integration_secondary_channel_feeds_policy(void)
{
    installSecondaryAt(1, "CHAT");
    const bool sec = channels.isConfiguredSecondary(1);
    const bool pri = channels.isConfiguredSecondary(0);
    TEST_ASSERT_TRUE(sec);
    TEST_ASSERT_FALSE(pri);
    TEST_ASSERT_FALSE(clientBaseAllowsCancelingDupe(false, sec));
    TEST_ASSERT_TRUE(clientBaseAllowsCancelingDupe(false, pri));
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
    exit(UNITY_END());
}

void loop() {}
