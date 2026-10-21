// CLIENT_BASE L2 secretary policy: D2 caps + enable gates (L2_PLAN P0).
//
// Pins ClientBaseSecretaryPolicy.h so STM32WL/TINY never enables the module,
// CLIENT_BASE alone enables on supported MEM_CLASS, and ring capacity stays
// within dump-time caps (never unbounded PSRAM S&F sizing).
//
// Does not construct MeshModule (that is firmware wiring); store init smoke
// uses the same cap/favorite/TTL constants the module constructor will pass.

#include "ClientBaseSecretaryPolicy.h"
#include "ClientBaseSecretaryStore.h"
#include "TestUtil.h"
#include "configuration.h"
#include "memory/MemClass.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

static void test_ttl_and_hard_caps_match_d2(void)
{
    TEST_ASSERT_EQUAL_UINT32(24UL * 3600UL, CLIENT_BASE_SECRETARY_TTL_SEC);
    TEST_ASSERT_EQUAL_UINT8(8, CLIENT_BASE_SECRETARY_MAX_FAVORITES);
    TEST_ASSERT_EQUAL_UINT16(32, CLIENT_BASE_SECRETARY_HISTORY_RETURN_MAX);
    TEST_ASSERT_EQUAL_UINT32(2500, CLIENT_BASE_SECRETARY_PACKET_INTERVAL_MS);
    TEST_ASSERT_TRUE(CLIENT_BASE_SECRETARY_RING_LARGE <= 256);
    TEST_ASSERT_TRUE(CLIENT_BASE_SECRETARY_RING_SMALL <= CLIENT_BASE_SECRETARY_RING_MEDIUM);
    TEST_ASSERT_TRUE(CLIENT_BASE_SECRETARY_RING_MEDIUM <= CLIENT_BASE_SECRETARY_RING_LARGE);
}

static void test_non_client_base_roles_disabled(void)
{
    TEST_ASSERT_FALSE(clientBaseSecretaryShouldEnable(meshtastic_Config_DeviceConfig_Role_CLIENT));
    TEST_ASSERT_FALSE(clientBaseSecretaryShouldEnable(meshtastic_Config_DeviceConfig_Role_CLIENT_MUTE));
    TEST_ASSERT_FALSE(clientBaseSecretaryShouldEnable(meshtastic_Config_DeviceConfig_Role_ROUTER));
    TEST_ASSERT_FALSE(clientBaseSecretaryShouldEnable(meshtastic_Config_DeviceConfig_Role_ROUTER_LATE));
}

static void test_client_base_enable_follows_platform(void)
{
    const bool want = clientBaseSecretaryPlatformSupported();
    TEST_ASSERT_EQUAL(want, clientBaseSecretaryShouldEnable(meshtastic_Config_DeviceConfig_Role_CLIENT_BASE));
#if MESHTASTIC_MEM_CLASS <= MEM_CLASS_TINY
    TEST_ASSERT_FALSE(want);
    TEST_ASSERT_EQUAL_UINT16(0, clientBaseSecretaryRingCapacity());
#else
    TEST_ASSERT_TRUE(want);
    TEST_ASSERT_TRUE(clientBaseSecretaryRingCapacity() > 0);
    TEST_ASSERT_TRUE(clientBaseSecretaryRingCapacity() <= CLIENT_BASE_SECRETARY_RING_LARGE);
#endif
}

static void test_store_init_with_policy_caps(void)
{
#if MESHTASTIC_MEM_CLASS <= MEM_CLASS_TINY
    TEST_ASSERT_EQUAL_UINT16(0, clientBaseSecretaryRingCapacity());
#else
    ClientBaseSecretaryStore store;
    TEST_ASSERT_TRUE(
        store.init(clientBaseSecretaryRingCapacity(), CLIENT_BASE_SECRETARY_MAX_FAVORITES, CLIENT_BASE_SECRETARY_TTL_SEC));
    TEST_ASSERT_EQUAL_UINT16(clientBaseSecretaryRingCapacity(), store.capacity());
    TEST_ASSERT_EQUAL_UINT8(CLIENT_BASE_SECRETARY_MAX_FAVORITES, store.maxFavorites());
    TEST_ASSERT_EQUAL_UINT32(CLIENT_BASE_SECRETARY_TTL_SEC, store.ttlSec());
    TEST_ASSERT_EQUAL_UINT16(0, store.size());
#endif
}

void setup()
{
    initializeTestEnvironment();
    UNITY_BEGIN();
    RUN_TEST(test_ttl_and_hard_caps_match_d2);
    RUN_TEST(test_non_client_base_roles_disabled);
    RUN_TEST(test_client_base_enable_follows_platform);
    RUN_TEST(test_store_init_with_policy_caps);
    exit(UNITY_END());
}

void loop() {}
