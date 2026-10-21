// CLIENT_BASE L2-P2 poll session: ACL, busy, multi-cursor, history_return_max.
//
// Pins ClientBaseSecretaryPoll.h used by ClientBaseSecretaryModule dump path.
// Does not exercise LoRa TX / StoreForward protobuf encode (module wiring).

#include "ClientBaseSecretaryPoll.h"
#include "ClientBaseSecretaryStore.h"
#include "TestUtil.h"
#include <cstring>
#include <unity.h>

static ClientBaseSecretaryStore *store = nullptr;
static ClientBaseSecretaryPollSession session;

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
    TEST_ASSERT_TRUE(store->init(16, 4, 3600));
    clientBaseSecretaryPollClear(session);
}

void tearDown(void)
{
    delete store;
    store = nullptr;
}

static void test_reject_non_favorite(void)
{
    store->push(makeRec(1, 1, 1, 10, "x"), 10);
    TEST_ASSERT_EQUAL((int)ClientBaseSecretaryPollStart::Rejected,
                      (int)clientBaseSecretaryBeginPoll(*store, session, 0xA1, false, 32));
    TEST_ASSERT_FALSE(session.busy);
}

static void test_empty_when_favorite_has_nothing(void)
{
    TEST_ASSERT_EQUAL((int)ClientBaseSecretaryPollStart::Empty,
                      (int)clientBaseSecretaryBeginPoll(*store, session, 0xA1, true, 32));
    TEST_ASSERT_FALSE(session.busy);
    TEST_ASSERT_TRUE(store->hasFavorite(0xA1));
}

static void test_started_pulls_and_advances_cursor_only(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    TEST_ASSERT_TRUE(store->ensureFavorite(0xB2));
    store->push(makeRec(1, 1, 1, 10, "m0"), 10);
    store->push(makeRec(1, 2, 1, 11, "m1"), 11);
    store->push(makeRec(1, 3, 1, 12, "m2"), 12);

    TEST_ASSERT_EQUAL((int)ClientBaseSecretaryPollStart::Started,
                      (int)clientBaseSecretaryBeginPoll(*store, session, 0xA1, true, 32));
    TEST_ASSERT_EQUAL_UINT16(3, session.remaining);

    ClientBaseLeanRecord out{};
    TEST_ASSERT_TRUE(clientBaseSecretaryPollPullNext(*store, session, out));
    TEST_ASSERT_EQUAL_UINT32(1, out.seq);
    TEST_ASSERT_EQUAL_UINT16(2, store->availableCount(0xA1));
    // Bart untouched
    TEST_ASSERT_EQUAL_UINT16(3, store->availableCount(0xB2));

    TEST_ASSERT_TRUE(clientBaseSecretaryPollPullNext(*store, session, out));
    TEST_ASSERT_TRUE(clientBaseSecretaryPollPullNext(*store, session, out));
    TEST_ASSERT_FALSE(session.busy);
    TEST_ASSERT_EQUAL_UINT16(3, session.sent);
    TEST_ASSERT_FALSE(clientBaseSecretaryPollPullNext(*store, session, out));
}

static void test_busy_blocks_second_poller(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    TEST_ASSERT_TRUE(store->ensureFavorite(0xB2));
    store->push(makeRec(1, 1, 1, 10, "m0"), 10);
    store->push(makeRec(1, 2, 1, 11, "m1"), 11);

    TEST_ASSERT_EQUAL((int)ClientBaseSecretaryPollStart::Started,
                      (int)clientBaseSecretaryBeginPoll(*store, session, 0xA1, true, 32));
    TEST_ASSERT_EQUAL((int)ClientBaseSecretaryPollStart::Busy,
                      (int)clientBaseSecretaryBeginPoll(*store, session, 0xB2, true, 32));
    TEST_ASSERT_EQUAL_UINT32(0xA1, session.busyTo);
    // Bart still has full history
    TEST_ASSERT_EQUAL_UINT16(2, store->availableCount(0xB2));
}

static void test_history_return_max_caps_session(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    for (uint32_t i = 0; i < 10; i++) {
        store->push(makeRec(1, i + 1, 1, 100 + i, "x"), 100 + i);
    }
    TEST_ASSERT_EQUAL((int)ClientBaseSecretaryPollStart::Started,
                      (int)clientBaseSecretaryBeginPoll(*store, session, 0xA1, true, /*max=*/3));
    TEST_ASSERT_EQUAL_UINT16(3, session.remaining);

    ClientBaseLeanRecord out{};
    TEST_ASSERT_TRUE(clientBaseSecretaryPollPullNext(*store, session, out));
    TEST_ASSERT_TRUE(clientBaseSecretaryPollPullNext(*store, session, out));
    TEST_ASSERT_TRUE(clientBaseSecretaryPollPullNext(*store, session, out));
    TEST_ASSERT_FALSE(session.busy);
    TEST_ASSERT_EQUAL_UINT16(7, store->availableCount(0xA1)); // 10-3 left for later poll
}

static void test_abort_only_owner(void)
{
    TEST_ASSERT_TRUE(store->ensureFavorite(0xA1));
    store->push(makeRec(1, 1, 1, 10, "m0"), 10);
    clientBaseSecretaryBeginPoll(*store, session, 0xA1, true, 32);
    TEST_ASSERT_FALSE(clientBaseSecretaryPollAbort(session, 0xB2));
    TEST_ASSERT_TRUE(session.busy);
    TEST_ASSERT_TRUE(clientBaseSecretaryPollAbort(session, 0xA1));
    TEST_ASSERT_FALSE(session.busy);
}

void setup()
{
    initializeTestEnvironment();
    UNITY_BEGIN();
    RUN_TEST(test_reject_non_favorite);
    RUN_TEST(test_empty_when_favorite_has_nothing);
    RUN_TEST(test_started_pulls_and_advances_cursor_only);
    RUN_TEST(test_busy_blocks_second_poller);
    RUN_TEST(test_history_return_max_caps_session);
    RUN_TEST(test_abort_only_owner);
    exit(UNITY_END());
}

void loop() {}
