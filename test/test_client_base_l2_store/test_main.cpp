// CLIENT_BASE L2 secretary store: shared ring + per-favorite cursors.
//
// Pins ClientBaseSecretaryStore against UC5 (multi-reader: one pull must not
// purge the ring for another favorite) and TTL eviction (UC6 offline window
// survives until expire, then drops). No purge-on-ACK - only cursor advance.
//
// Does not cover poll protobuf / LoRa unicast path (separate suite later).

#include "ClientBaseSecretaryStore.h"
#include "TestUtil.h"
#include <cstring>
#include <unity.h>

static ClientBaseSecretaryStore *store = nullptr;

static ClientBaseLeanRecord makeRec(uint32_t from, uint32_t id, uint8_t channel, uint32_t timeSec, const char *text)
{
    ClientBaseLeanRecord r{};
    r.from = from;
    r.id = id;
    r.channel = channel;
    r.time_sec = timeSec;
    if (text) {
        const size_t n = strlen(text);
        r.payload_len = static_cast<uint8_t>(n > CLIENT_BASE_LEAN_PAYLOAD_MAX ? CLIENT_BASE_LEAN_PAYLOAD_MAX : n);
        memcpy(r.payload, text, r.payload_len);
    }
    return r;
}

void setUp(void)
{
    store = new ClientBaseSecretaryStore();
    TEST_ASSERT_TRUE(store->init(/*capacity=*/8, /*maxFavorites=*/4, /*ttlSec=*/3600));
}

void tearDown(void)
{
    delete store;
    store = nullptr;
}

static void test_lean_record_is_256_bytes(void)
{
    TEST_ASSERT_EQUAL(256, (int)sizeof(ClientBaseLeanRecord));
}

static void test_push_and_available_for_favorite(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    TEST_ASSERT_EQUAL_UINT32(1, store->push(makeRec(0x10, 1, 1, 100, "a"), 100));
    TEST_ASSERT_EQUAL_UINT32(2, store->push(makeRec(0x10, 2, 1, 101, "b"), 101));
    TEST_ASSERT_EQUAL_UINT16(2, store->availableCount(0xA1));
    TEST_ASSERT_EQUAL_UINT16(0, store->availableCount(0xB2)); // not registered
}

static void test_multi_reader_no_purge_on_pull(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    TEST_ASSERT_TRUE(store->ensureFavorite(0xB2));
    store->push(makeRec(1, 1, 1, 10, "m0"), 10);
    store->push(makeRec(1, 2, 1, 11, "m1"), 11);
    store->push(makeRec(1, 3, 1, 12, "m2"), 12);

    ClientBaseLeanRecord out{};
    TEST_ASSERT_TRUE(store->pullNext(0xA1, out));
    TEST_ASSERT_EQUAL_UINT32(1, out.seq);
    TEST_ASSERT_EQUAL_UINT16(2, store->availableCount(0xA1));
    // Bart still sees all three - Anna's pull did not delete
    TEST_ASSERT_EQUAL_UINT16(3, store->availableCount(0xB2));

    TEST_ASSERT_TRUE(store->pullNext(0xB2, out));
    TEST_ASSERT_EQUAL_UINT32(1, out.seq);
    TEST_ASSERT_TRUE(store->pullNext(0xB2, out));
    TEST_ASSERT_EQUAL_UINT32(2, out.seq);
    TEST_ASSERT_EQUAL_UINT16(2, store->availableCount(0xA1));
    TEST_ASSERT_EQUAL_UINT16(1, store->availableCount(0xB2));
}

static void test_ttl_expires_oldest(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    // Keep all pushes inside the TTL window so push()-time expire does not drop them.
    store->push(makeRec(1, 1, 1, 100, "old"), 100);
    store->push(makeRec(1, 2, 1, 200, "mid"), 200);
    store->push(makeRec(1, 3, 1, 300, "new"), 300);
    TEST_ASSERT_EQUAL_UINT16(3, store->size());

    // ttl=3600: age >= ttl drops. At t=3700, @100 (3600s) drops; @200 (3500s) stays.
    store->expire(3700);
    TEST_ASSERT_EQUAL_UINT16(2, store->size());
    TEST_ASSERT_EQUAL_UINT16(2, store->availableCount(0xA1));

    ClientBaseLeanRecord out{};
    TEST_ASSERT_TRUE(store->peekNext(0xA1, out));
    TEST_ASSERT_EQUAL_UINT32(2, out.id); // mid survived; old gone
}

static void test_ring_capacity_drops_oldest(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    for (uint32_t i = 0; i < 10; i++)
        store->push(makeRec(1, i + 1, 1, 100 + i, "x"), 100 + i);
    TEST_ASSERT_EQUAL_UINT16(8, store->size());
    ClientBaseLeanRecord out{};
    TEST_ASSERT_TRUE(store->peekNext(0xA1, out));
    // first two dropped (ids 1,2); oldest live is id 3
    TEST_ASSERT_EQUAL_UINT32(3, out.id);
}

static void test_favorite_cap(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(1));
    TEST_ASSERT_TRUE(store->ensureFavorite(2));
    TEST_ASSERT_TRUE(store->ensureFavorite(3));
    TEST_ASSERT_TRUE(store->ensureFavorite(4));
    TEST_ASSERT_FALSE(store->ensureFavorite(5));
    TEST_ASSERT_TRUE(store->ensureFavorite(1)); // idempotent
}

static void test_pull_advances_only_that_cursor(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    store->push(makeRec(1, 9, 1, 1, "only"), 1);
    ClientBaseLeanRecord a{}, b{};
    TEST_ASSERT_TRUE(store->pullNext(0xA1, a));
    TEST_ASSERT_FALSE(store->pullNext(0xA1, b));
    TEST_ASSERT_EQUAL_UINT16(0, store->availableCount(0xA1));
    TEST_ASSERT_EQUAL_UINT16(1, store->size()); // still in ring
}

void setup()
{
    initializeTestEnvironment();
    UNITY_BEGIN();
    RUN_TEST(test_lean_record_is_256_bytes);
    RUN_TEST(test_push_and_available_for_favorite);
    RUN_TEST(test_multi_reader_no_purge_on_pull);
    RUN_TEST(test_ttl_expires_oldest);
    RUN_TEST(test_ring_capacity_drops_oldest);
    RUN_TEST(test_favorite_cap);
    RUN_TEST(test_pull_advances_only_that_cursor);
    exit(UNITY_END());
}

void loop() {}
