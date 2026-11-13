#include <unity.h>
#include "http_response.h"
#include <algorithm>
#include <string>

static FirmnginHttpBodyResult readBody(const std::string &wire, String &body,
                                      size_t limit, bool complete = true, bool error = false)
{
    size_t offset = 0;
    return firmngin_read_http_body(
        [&](char *data, size_t capacity) {
            if (offset == wire.size()) return error ? -1 : 0;
            const size_t count = std::min<size_t>(13, std::min(capacity, wire.size() - offset));
            memcpy(data, wire.data() + offset, count);
            offset += count;
            return static_cast<int>(count);
        },
        [&]() { return complete && offset == wire.size(); }, body, limit);
}

void test_success_keeps_a_complete_long_json_response()
{
    const std::string wire = "{\"image_url\":\"" + std::string(600, 'x') + "\"}";
    String body;
    TEST_ASSERT_TRUE(readBody(wire, body, 4096) == FirmnginHttpBodyResult::Complete);
    TEST_ASSERT_EQUAL_STRING(wire.c_str(), body.c_str());
}

void test_exact_limit_is_allowed_and_overflow_is_rejected()
{
    String body;
    TEST_ASSERT_TRUE(readBody(std::string(4096, 'x'), body, 4096) == FirmnginHttpBodyResult::Complete);
    TEST_ASSERT_EQUAL_UINT(4096, body.length());
    TEST_ASSERT_TRUE(readBody(std::string(4097, 'x'), body, 4096) == FirmnginHttpBodyResult::TooLarge);
    TEST_ASSERT_EQUAL_UINT(0, body.length());
}

void test_eof_without_http_completion_does_not_return_partial_json()
{
    String body = "old response";
    TEST_ASSERT_TRUE(readBody("{\"image_url\":", body, 4096, false) == FirmnginHttpBodyResult::Incomplete);
    TEST_ASSERT_EQUAL_UINT(0, body.length());
}

void test_transport_error_after_bytes_is_not_success()
{
    String body;
    TEST_ASSERT_TRUE(readBody("{\"ok\":true}", body, 4096, true, true) == FirmnginHttpBodyResult::Incomplete);
    TEST_ASSERT_EQUAL_UINT(0, body.length());
}

void test_empty_complete_response_is_allowed()
{
    String body;
    TEST_ASSERT_TRUE(readBody("", body, 4096) == FirmnginHttpBodyResult::Complete);
}

void test_insecure_mode_requires_explicit_sdk_support()
{
    TEST_ASSERT_NULL(firmngin_http_security_error(false, true, false));
    TEST_ASSERT_NOT_NULL(firmngin_http_security_error(false, false, true));
    TEST_ASSERT_NOT_NULL(firmngin_http_security_error(true, true, false));
    TEST_ASSERT_NOT_NULL(firmngin_http_security_error(true, false, false));
    TEST_ASSERT_NULL(firmngin_http_security_error(true, false, true));
}

void setUp() {}
void tearDown() {}
int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_success_keeps_a_complete_long_json_response);
    RUN_TEST(test_exact_limit_is_allowed_and_overflow_is_rejected);
    RUN_TEST(test_eof_without_http_completion_does_not_return_partial_json);
    RUN_TEST(test_transport_error_after_bytes_is_not_success);
    RUN_TEST(test_empty_complete_response_is_allowed);
    RUN_TEST(test_insecure_mode_requires_explicit_sdk_support);
    return UNITY_END();
}
