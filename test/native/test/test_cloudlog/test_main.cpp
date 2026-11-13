#include <unity.h>

#include "firmngin_cloudlog.h"
#include <string>
#include <vector>

// CloudLog is the fngin.print/println engine: echo to Serial (stdout here)
// and buffer for the platform MQTT drain. These tests drive that buffering
// and drain path directly with a capture sink.

static std::vector<std::string> capLines;
static std::vector<unsigned long> capTs;
static bool capAccept = true;

static bool captureSink(const char *line, unsigned long tsMs, void *user)
{
    (void)user;
    if (!capAccept)
        return false;
    capLines.push_back(line);
    capTs.push_back(tsMs);
    return true;
}

static void captureSinkReset()
{
    capLines.clear();
    capTs.clear();
    capAccept = true;
}

void test_print_then_println_and_drain_order()
{
    CloudLog log;
    log.begin(nullptr);
    log.onSink(captureSink);
    log.clear();

    log.print("counter=");
    log.print(42);
    log.println(" ready");
    log.println("second");

    TEST_ASSERT_EQUAL_size_t(2, log.pendingLines());
    TEST_ASSERT_EQUAL_size_t(2, log.drain());
    TEST_ASSERT_EQUAL_size_t(0, log.pendingLines());
    TEST_ASSERT_EQUAL_STRING("counter=42 ready", capLines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("second", capLines[1].c_str());
}

void test_drain_reject_preserves_order()
{
    CloudLog log;
    log.begin(nullptr);
    log.onSink(captureSink);
    log.clear();

    log.println("first");
    log.println("second");
    capAccept = false;
    TEST_ASSERT_EQUAL_size_t(0, log.drain());
    TEST_ASSERT_EQUAL_size_t(2, log.pendingLines());

    capAccept = true;
    TEST_ASSERT_EQUAL_size_t(2, log.drain());
    TEST_ASSERT_EQUAL_STRING("first", capLines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("second", capLines[1].c_str());
}

void test_sink_null_rejects()
{
    CloudLog log;
    log.begin(nullptr); // no sink wired yet
    log.clear();
    log.println("waiting");

    TEST_ASSERT_EQUAL_size_t(0, log.drain());
    TEST_ASSERT_EQUAL_size_t(1, log.pendingLines());

    log.onSink(captureSink);
    TEST_ASSERT_EQUAL_size_t(1, log.drain());
    TEST_ASSERT_EQUAL_STRING("waiting", capLines[0].c_str());
}

void test_overflow_counts_dropped_lines()
{
    CloudLog log;
    log.begin(nullptr);
    log.onSink(captureSink);
    log.clear();

    // Never drain: the ring (8 x 200 = 1600 B) fills up at 24-line segment
    // cap first — push more lines than that with long-ish content.
    for (int i = 0; i < 60; i++)
        log.println("line overflow test 1234567890");
    TEST_ASSERT_TRUE(log.dropped() > 0);
    TEST_ASSERT_TRUE(log.pendingLines() <= 24);

    TEST_ASSERT_EQUAL_size_t(24, log.drain());
}

void test_printf_formats_like_serial()
{
    CloudLog log;
    log.begin(nullptr);
    log.onSink(captureSink);
    log.clear();

    log.printf("temp=%d status=%s\n", 33, "ok");
    TEST_ASSERT_EQUAL_size_t(1, log.pendingLines());
    TEST_ASSERT_EQUAL_size_t(1, log.drain());
    TEST_ASSERT_EQUAL_STRING("temp=33 status=ok\n", capLines[0].c_str());
}

void test_lifetime_clock_flows_to_sink()
{
    static unsigned long fakeNow = 1234UL;
    CloudLog log;
    log.begin([]() { return fakeNow; });
    log.onSink(captureSink);
    log.clear();

    log.println("ts check");
    fakeNow = 5678UL;
    log.println("ts check 2");
    log.drain();
    TEST_ASSERT_EQUAL_UINT32(1234, capTs[0]);
    TEST_ASSERT_EQUAL_UINT32(5678, capTs[1]);
}

void test_noop_before_begin()
{
    CloudLog log;
    log.onSink(captureSink); // sink wired but begin() not called yet
    log.clear();

    log.print("setup");
    log.println("early");
    log.printf("x=%d\n", 1);
    TEST_ASSERT_EQUAL_size_t(0, log.pendingLines());
    TEST_ASSERT_EQUAL_size_t(0, log.drain());
    TEST_ASSERT_EQUAL_size_t(0, capLines.empty() ? 0 : capLines.size());
    TEST_ASSERT_FALSE(log.armed());

    log.begin(nullptr); // arm it
    TEST_ASSERT_TRUE(log.armed());
    log.println("after begin");
    TEST_ASSERT_EQUAL_size_t(1, log.pendingLines());
    TEST_ASSERT_EQUAL_size_t(1, log.drain());
    TEST_ASSERT_EQUAL_STRING("after begin", capLines[0].c_str());
}

void setUp(void) { captureSinkReset(); }
void tearDown(void) {}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_print_then_println_and_drain_order);
    RUN_TEST(test_drain_reject_preserves_order);
    RUN_TEST(test_sink_null_rejects);
    RUN_TEST(test_overflow_counts_dropped_lines);
    RUN_TEST(test_printf_formats_like_serial);
    RUN_TEST(test_lifetime_clock_flows_to_sink);
    RUN_TEST(test_noop_before_begin);
    return UNITY_END();
}
