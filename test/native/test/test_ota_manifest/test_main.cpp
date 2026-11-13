#include <unity.h>

#include "ota_manifest.h"
#include "ota_progress.h"
#include "ota_rollback.h"

void test_valid_sha256_accepted()
{
    String h = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    TEST_ASSERT_TRUE(isValidOTAHash(h));
}

void test_short_sha256_rejected()
{
    String h = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde";
    TEST_ASSERT_FALSE(isValidOTAHash(h));
}

void test_long_sha256_rejected()
{
    String h = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdefff";
    TEST_ASSERT_FALSE(isValidOTAHash(h));
}

void test_uppercase_sha256_accepted()
{
    String h = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
    TEST_ASSERT_TRUE(isValidOTAHash(h));
}

void test_non_hex_sha256_rejected()
{
    String h = "0123456789ggdef0123456789abcdef0123456789abcdef0123456789abcdef";
    TEST_ASSERT_FALSE(isValidOTAHash(h));
}

void test_valid_firmware_id_accepted()
{
    TEST_ASSERT_TRUE(isValidFirmwareID("2f6a3c1e-8b4d-4e2a-9c10-7a55d1e0f2b3"));
}

void test_short_firmware_id_rejected()
{
    TEST_ASSERT_FALSE(isValidFirmwareID("2f6a3c1e-8b4d-4e2a-9c10-7a55d1e0f2b"));
}

void test_wrong_dash_positions_rejected()
{
    // dash at index 7 instead of 8
    TEST_ASSERT_FALSE(isValidFirmwareID("2f6a3c1-e8b4d-4e2a-9c10-7a55d1e0f2b3"));
}

void test_non_hex_firmware_id_rejected()
{
    TEST_ASSERT_FALSE(isValidFirmwareID("2f6a3c1e-8b4d-4e2a-9c10-7a55d1e0f2bg"));
}

void test_null_firmware_id_rejected()
{
    TEST_ASSERT_FALSE(isValidFirmwareID(nullptr));
}

void test_query_params_encode_specials()
{
    char query[64] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(appendOTAQueryParameter(query, sizeof(query), length, "v", "1 2&3"));
    // space -> %20, & -> %26
    TEST_ASSERT_EQUAL_STRING("v=1%202%263", query);
}

void test_query_params_first_needs_no_separator()
{
    char query[16] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(appendOTAQueryParameter(query, sizeof(query), length, "a", "b"));
    TEST_ASSERT_EQUAL_STRING("a=b", query);
}

void test_query_params_chain_with_ampersand()
{
    char query[32] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(appendOTAQueryParameter(query, sizeof(query), length, "a", "1"));
    TEST_ASSERT_TRUE(appendOTAQueryParameter(query, sizeof(query), length, "b", "2"));
    TEST_ASSERT_EQUAL_STRING("a=1&b=2", query);
}

void test_query_params_overflow_rejected()
{
    char query[8] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(appendOTAQueryParameter(query, sizeof(query), length, "a", "1"));
    // b=22 does not fit into 8 bytes
    TEST_ASSERT_FALSE(appendOTAQueryParameter(query, sizeof(query), length, "b", "22"));
}

void test_empty_value_encodes_nothing()
{
    char query[16] = {};
    size_t length = 0;
    TEST_ASSERT_TRUE(appendOTAQueryParameter(query, sizeof(query), length, "k", ""));
    TEST_ASSERT_EQUAL_STRING("k=", query);
}

void test_stall_not_exceeded_before_budget()
{
    TEST_ASSERT_FALSE(otaStallExceeded(14000UL, 0UL, 15000UL));
}

void test_stall_exceeded_after_budget()
{
    TEST_ASSERT_TRUE(otaStallExceeded(16000UL, 0UL, 15000UL));
}

void test_stall_check_is_wrap_safe()
{
    // millis() wraps after ~49.7 days on the 32-bit device (unsigned long is
    // 32-bit there, 64-bit on this host). The constants below carry the same
    // verdict whichever width: wrap-true (66536 ms), wrap-false (1016 ms).
    TEST_ASSERT_TRUE(otaStallExceeded(1000UL, 0xFFFFFFFFFFFF0000UL, 15000UL));  // far past wrap
    TEST_ASSERT_FALSE(otaStallExceeded(1000UL, 0xFFFFFFFFFFFFFFF0UL, 15000UL)); // just past wrap
}

struct ReportStore
{
    bool report = false;
    bool message = false;
    bool failErase = false;
    int erases = 0;

    bool isKey(const char *key) const
    {
        return strcmp(key, "boot_report") == 0 ? report : message;
    }

    bool remove(const char *key)
    {
        ++erases;
        if (failErase || !isKey(key))
            return false; // Match Arduino Preferences, not the permissive IDF shim.
        (strcmp(key, "boot_report") == 0 ? report : message) = false;
        return true;
    }
};

void test_missing_boot_report_needs_no_erase()
{
    ReportStore store;
    TEST_ASSERT_TRUE(firmngin_clear_ota_boot_report(store));
    TEST_ASSERT_EQUAL_INT(0, store.erases);
}

void test_partial_boot_report_clears_and_can_be_cleared_again()
{
    ReportStore store;
    store.message = true;
    TEST_ASSERT_TRUE(firmngin_clear_ota_boot_report(store));
    TEST_ASSERT_FALSE(store.message);
    TEST_ASSERT_TRUE(firmngin_clear_ota_boot_report(store));
    TEST_ASSERT_EQUAL_INT(1, store.erases);
}

void test_boot_report_erase_errors_are_not_ignored()
{
    ReportStore store;
    store.report = store.message = true;
    store.failErase = true;
    TEST_ASSERT_FALSE(firmngin_clear_ota_boot_report(store));
    TEST_ASSERT_TRUE(store.report);
    TEST_ASSERT_TRUE(store.message);
    store.failErase = false;
    TEST_ASSERT_TRUE(firmngin_clear_ota_boot_report(store));
    TEST_ASSERT_FALSE(store.report);
    TEST_ASSERT_FALSE(store.message);
}

void test_stall_failure_allows_a_new_download_but_busy_states_do_not()
{
    OTAAsyncState state = OTA_ASYNC_DOWNLOADING;
    TEST_ASSERT_FALSE(firmngin_ota_can_start(state));
    TEST_ASSERT_TRUE(otaStallExceeded(16000UL, 0UL, 15000UL));
    state = OTA_ASYNC_FAILED; // _otaCleanup() releases the failed transfer.
    TEST_ASSERT_TRUE(firmngin_ota_can_start(state));
    state = OTA_ASYNC_DOWNLOADING; // The next request starts a new transfer.
    TEST_ASSERT_FALSE(firmngin_ota_can_start(state));
    TEST_ASSERT_FALSE(firmngin_ota_can_start(OTA_ASYNC_VERIFYING));
    TEST_ASSERT_FALSE(firmngin_ota_can_start(OTA_ASYNC_INSTALLING));
    TEST_ASSERT_TRUE(firmngin_ota_can_start(OTA_ASYNC_IDLE));
}

void test_signature_headers_match_any_http_case()
{
    TEST_ASSERT_TRUE(otaHeaderMatches("X-Firmware-Signature", "X-Firmware-Signature"));
    TEST_ASSERT_TRUE(otaHeaderMatches("x-firmware-signature", "X-Firmware-Signature"));
    TEST_ASSERT_TRUE(otaHeaderMatches("x-FIRMWARE-key-sha256", "X-Firmware-Key-Sha256"));
    TEST_ASSERT_FALSE(otaHeaderMatches("x-firmware-signature-extra", "X-Firmware-Signature"));
    TEST_ASSERT_FALSE(otaHeaderMatches(nullptr, "X-Firmware-Signature"));
}

void test_manifest_parser_clears_old_artifacts_on_invalid_or_no_update()
{
    FirmnginOtaManifest manifest;
    const String body = String("{\"firmware_id\":\"2f6a3c1e-8b4d-4e2a-9c10-7a55d1e0f2b3\",\"version\":\"1.4.0\",\"sha256\":\"") +
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\"}";
    TEST_ASSERT_TRUE(firmngin_parse_ota_manifest(body, manifest) == FirmnginOtaManifestResult::Available);
    TEST_ASSERT_EQUAL_STRING("1.4.0", manifest.version.c_str());
    TEST_ASSERT_TRUE(firmngin_parse_ota_manifest("{\"data\":null}", manifest) == FirmnginOtaManifestResult::UpToDate);
    TEST_ASSERT_EQUAL_UINT(0, manifest.id.length());
    manifest.id = "old artifact";
    TEST_ASSERT_TRUE(firmngin_parse_ota_manifest("{\"firmware_id\":\"invalid\"}", manifest) == FirmnginOtaManifestResult::Invalid);
    TEST_ASSERT_EQUAL_UINT(0, manifest.id.length());
}

void test_worker_manifest_query_is_encoded_and_bounded()
{
    String path;
    TEST_ASSERT_TRUE(firmngin_ota_manifest_path("ESP32", "a b", "1.4.0", path));
    TEST_ASSERT_EQUAL_STRING("/manifest?target_board=ESP32&target_model=a%20b&current_version=1.4.0", path.c_str());
    TEST_ASSERT_FALSE(firmngin_ota_manifest_path("ESP32", String(std::string(600, 'x')), "1.4.0", path));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_sha256_accepted);
    RUN_TEST(test_short_sha256_rejected);
    RUN_TEST(test_long_sha256_rejected);
    RUN_TEST(test_uppercase_sha256_accepted);
    RUN_TEST(test_non_hex_sha256_rejected);
    RUN_TEST(test_valid_firmware_id_accepted);
    RUN_TEST(test_short_firmware_id_rejected);
    RUN_TEST(test_wrong_dash_positions_rejected);
    RUN_TEST(test_non_hex_firmware_id_rejected);
    RUN_TEST(test_null_firmware_id_rejected);
    RUN_TEST(test_query_params_encode_specials);
    RUN_TEST(test_query_params_first_needs_no_separator);
    RUN_TEST(test_query_params_chain_with_ampersand);
    RUN_TEST(test_query_params_overflow_rejected);
    RUN_TEST(test_empty_value_encodes_nothing);
    RUN_TEST(test_stall_not_exceeded_before_budget);
    RUN_TEST(test_stall_exceeded_after_budget);
    RUN_TEST(test_stall_check_is_wrap_safe);
    RUN_TEST(test_missing_boot_report_needs_no_erase);
    RUN_TEST(test_partial_boot_report_clears_and_can_be_cleared_again);
    RUN_TEST(test_boot_report_erase_errors_are_not_ignored);
    RUN_TEST(test_stall_failure_allows_a_new_download_but_busy_states_do_not);
    RUN_TEST(test_signature_headers_match_any_http_case);
    RUN_TEST(test_manifest_parser_clears_old_artifacts_on_invalid_or_no_update);
    RUN_TEST(test_worker_manifest_query_is_encoded_and_bounded);
    return UNITY_END();
}
