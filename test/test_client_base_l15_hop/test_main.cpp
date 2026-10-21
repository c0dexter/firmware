// CLIENT_BASE L1.5 free-hop / last-meter (#8366 design).
//
// Pins ClientBaseHop.h:
// - Direct from favorite (hopsAway==0): FreeHopBumpStart (keep hop_limit, caller bumps hop_start)
// - hop_limit==0 to/from favorite: DeliverAtZero (last meter, no invented budget)
// - Otherwise NormalDecrement
//
// Guards against attic CLIENT_BASE chains inventing hop budget while still serving
// household favorites at the start and end of the path.

#include "ClientBaseHop.h"
#include "TestUtil.h"
#include "configuration.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

static void test_non_client_base_always_normal(void)
{
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::NormalDecrement, (int)clientBaseClassifyRelayHop(false, true, true, 0, 3));
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::NormalDecrement, (int)clientBaseClassifyRelayHop(false, true, false, 0, 0));
}

static void test_direct_from_favorite_is_free_hop(void)
{
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::FreeHopBumpStart,
                      (int)clientBaseClassifyRelayHop(true, /*fromFav=*/true, /*toFav=*/false, /*hopsAway=*/0, 3));
    // Both ends favorite + direct still free-hop (start of path wins)
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::FreeHopBumpStart, (int)clientBaseClassifyRelayHop(true, true, true, 0, 3));
}

static void test_direct_from_favorite_at_zero_is_still_free_hop(void)
{
    // Classify stays FreeHopBumpStart; NextHopRouter mayRelay must allow it (not only DeliverAtZero).
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::FreeHopBumpStart,
                      (int)clientBaseClassifyRelayHop(true, true, false, 0, /*hopLimit=*/0));
}

static void test_indirect_from_favorite_decrements(void)
{
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::NormalDecrement,
                      (int)clientBaseClassifyRelayHop(true, true, false, /*hopsAway=*/1, 2));
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::NormalDecrement, (int)clientBaseClassifyRelayHop(true, true, false, 2, 1));
}

static void test_last_meter_to_favorite_at_zero(void)
{
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::DeliverAtZero,
                      (int)clientBaseClassifyRelayHop(true, false, /*toFav=*/true, 3, /*hopLimit=*/0));
}

static void test_last_meter_from_favorite_at_zero(void)
{
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::DeliverAtZero,
                      (int)clientBaseClassifyRelayHop(true, /*fromFav=*/true, false, 3, 0));
}

static void test_zero_hop_without_favorite_is_normal(void)
{
    // Public mesh exhausted - do not invent CLIENT_BASE amplification
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::NormalDecrement, (int)clientBaseClassifyRelayHop(true, false, false, 3, 0));
}

static void test_non_favorite_direct_decrements(void)
{
    TEST_ASSERT_EQUAL((int)ClientBaseRelayHop::NormalDecrement, (int)clientBaseClassifyRelayHop(true, false, false, 0, 3));
}

void setup()
{
    initializeTestEnvironment();
    UNITY_BEGIN();
    RUN_TEST(test_non_client_base_always_normal);
    RUN_TEST(test_direct_from_favorite_is_free_hop);
    RUN_TEST(test_direct_from_favorite_at_zero_is_still_free_hop);
    RUN_TEST(test_indirect_from_favorite_decrements);
    RUN_TEST(test_last_meter_to_favorite_at_zero);
    RUN_TEST(test_last_meter_from_favorite_at_zero);
    RUN_TEST(test_zero_hop_without_favorite_is_normal);
    RUN_TEST(test_non_favorite_direct_decrements);
    exit(UNITY_END());
}

void loop() {}
