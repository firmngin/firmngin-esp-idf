#include <unity.h>

#include "identity_meta.h"

void test_trim_strips_spaces_and_newline()
{
    TEST_ASSERT_EQUAL_STRING("value", fng_trim(String(" value \n")).c_str());
}

void test_split_meta_line()
{
    String key, value;
    TEST_ASSERT_TRUE(splitMetaLine(String("device_id=abc"), key, value));
    TEST_ASSERT_EQUAL_STRING("device_id", key.c_str());
    TEST_ASSERT_EQUAL_STRING("abc", value.c_str());
}

void test_split_meta_line_trims()
{
    String key, value;
    TEST_ASSERT_TRUE(splitMetaLine(String(" tls = ca "), key, value));
    TEST_ASSERT_EQUAL_STRING("tls", key.c_str());
    TEST_ASSERT_EQUAL_STRING("ca", value.c_str());
}

void test_split_meta_line_no_key_rejected()
{
    String key, value;
    TEST_ASSERT_FALSE(splitMetaLine(String("=value"), key, value));
}

void test_https_url_accepts_valid()
{
    TEST_ASSERT_TRUE(isValidHttpsBaseUrl(String("https://ota.firmngin.dev/api/v1")));
    TEST_ASSERT_TRUE(isValidHttpsBaseUrl(String("https://ota.firmngin.dev")));
}

void test_https_url_rejects_trailing_slash()
{
    TEST_ASSERT_FALSE(isValidHttpsBaseUrl(String("https://ota.firmngin.dev/api/v1/")));
}

void test_https_url_rejects_plain_http()
{
    TEST_ASSERT_FALSE(isValidHttpsBaseUrl(String("http://ota.firmngin.dev")));
}

void test_https_url_rejects_userinfo()
{
    TEST_ASSERT_FALSE(isValidHttpsBaseUrl(String("https://user@ota.firmngin.dev/api")));
}

void test_https_url_rejects_query_or_space()
{
    TEST_ASSERT_FALSE(isValidHttpsBaseUrl(String("https://ota.firmngin.dev/api?a=1")));
    TEST_ASSERT_FALSE(isValidHttpsBaseUrl(String("https://ota.firmngin.dev/api x")));
}

void test_stream_endpoint_accepts_wss_and_ws()
{
    TEST_ASSERT_TRUE(isValidStreamEndpoint(String("wss://astream.firmngin.dev/v1")));
    TEST_ASSERT_TRUE(isValidStreamEndpoint(String("ws://192.168.1.5:9000/stream")));
}

void test_stream_endpoint_rejects_ftp()
{
    TEST_ASSERT_FALSE(isValidStreamEndpoint(String("ftp://astream.firmngin.dev/v1")));
}

void test_stream_endpoint_rejects_empty_authority()
{
    TEST_ASSERT_FALSE(isValidStreamEndpoint(String("wss:///v1")));
}

void test_stream_endpoint_rejects_userinfo()
{
    TEST_ASSERT_FALSE(isValidStreamEndpoint(String("wss://user@astream.firmngin.dev/v1")));
}

void test_find_meta_value_returns_match()
{
    String meta = "device_id=abc\ntls=ca\nbroker_port=58884\n";
    String out;
    TEST_ASSERT_TRUE(findMetaValue(meta, "tls", out));
    TEST_ASSERT_EQUAL_STRING("ca", out.c_str());
    TEST_ASSERT_TRUE(findMetaValue(meta, "broker_port", out));
    TEST_ASSERT_EQUAL_STRING("58884", out.c_str());
    TEST_ASSERT_FALSE(findMetaValue(meta, "missing", out));
}

void test_parse_meta_full_record()
{
    FirmnginIdentityRecord out;
    String meta = "device_id=d1\ndevice_key=k1\ndecryptor=00112233445566778899aabbccddeeff\n"
                  "tls=ca\nbroker=asia-jkt1.firmngin.dev\nbroker_port=58884\n"
                  "api_base_url=https://api.firmngin.dev/api/v1\n"
                  "ota_base_url=https://ota.firmngin.dev/api/v1/ota\n";
    TEST_ASSERT_TRUE(parseMeta(meta, out));
    TEST_ASSERT_EQUAL_STRING("d1", out.deviceId.c_str());
    TEST_ASSERT_EQUAL_INT(58884, out.brokerPort);
    TEST_ASSERT_EQUAL_STRING("https://api.firmngin.dev/api/v1", out.apiBaseUrl.c_str());
}

void test_parse_meta_rejects_bad_endpoint()
{
    FirmnginIdentityRecord out;
    String meta = "device_id=d1\ndevice_key=k1\ndecryptor=00112233445566778899aabbccddeeff\n"
                  "tls=ca\napi_base_url=https://bad.api/\notа_base_url=https://ota.firmngin.dev\n";
    TEST_ASSERT_FALSE(parseMeta(meta, out));
}

void test_validate_record_accepts_complete()
{
    FirmnginIdentityRecord r;
    r.deviceId = "d1";
    r.deviceKey = "k1";
    r.decryptor = "00112233445566778899aabbccddeeff";
    r.tlsMode = "fingerprint";
    r.fingerprintHex = "0011223344556677889900112233445566778899";
    r.apiBaseUrl = "https://api.firmngin.dev/api/v1";
    r.otaBaseUrl = "https://ota.firmngin.dev/api/v1/ota";
    r.clientCert = "-----BEGIN CERTIFICATE-----\nx\n-----END CERTIFICATE-----";
    r.privateKey = "-----BEGIN PRIVATE KEY-----\nx\n-----END PRIVATE KEY-----";
    r.serviceCaCert = "-----BEGIN CERTIFICATE-----\nx\n-----END CERTIFICATE-----";
    TEST_ASSERT_TRUE(validateIdentityRecord(r));
}

void test_validate_record_rejects_wrong_decryptor_length()
{
    FirmnginIdentityRecord r;
    r.deviceId = "d1";
    r.deviceKey = "k1";
    r.decryptor = "";
    r.tlsMode = "fingerprint";
    r.fingerprintHex = "0011223344556677889900112233445566778899";
    r.apiBaseUrl = "https://api.firmngin.dev/api/v1";
    r.otaBaseUrl = "https://ota.firmngin.dev/api/v1/ota";
    r.clientCert = "-----BEGIN CERTIFICATE-----\nx\n-----END CERTIFICATE-----";
    r.privateKey = "-----BEGIN PRIVATE KEY-----\nx\n-----END PRIVATE KEY-----";
    r.serviceCaCert = "-----BEGIN CERTIFICATE-----\nx\n-----END CERTIFICATE-----";
    TEST_ASSERT_FALSE(validateIdentityRecord(r));
}

void test_runtime_identity_allows_explicitly_disabled_e2ee_but_not_bad_tls()
{
    FirmnginIdentityRecord r;
    r.deviceId = "d1";
    r.deviceKey = "k1";
    r.tlsMode = "ca";
    r.apiBaseUrl = "https://api.firmngin.dev/api/v1";
    r.otaBaseUrl = "https://ota.firmngin.dev/api/v1/ota";
    r.caCert = r.clientCert = r.serviceCaCert = "-----BEGIN CERTIFICATE-----\nx\n-----END CERTIFICATE-----";
    r.privateKey = "-----BEGIN PRIVATE KEY-----\nx\n-----END PRIVATE KEY-----";
    TEST_ASSERT_TRUE(validateIdentityRecord(r, false));
    TEST_ASSERT_FALSE(validateIdentityRecord(r)); // Factory provisioning remains strict.
    r.decryptor = "not hex";
    TEST_ASSERT_FALSE(validateIdentityRecord(r, false));
    r.decryptor = "";
    r.serviceCaCert = "";
    TEST_ASSERT_FALSE(validateIdentityRecord(r, false));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_trim_strips_spaces_and_newline);
    RUN_TEST(test_split_meta_line);
    RUN_TEST(test_split_meta_line_trims);
    RUN_TEST(test_split_meta_line_no_key_rejected);
    RUN_TEST(test_https_url_accepts_valid);
    RUN_TEST(test_https_url_rejects_trailing_slash);
    RUN_TEST(test_https_url_rejects_plain_http);
    RUN_TEST(test_https_url_rejects_userinfo);
    RUN_TEST(test_https_url_rejects_query_or_space);
    RUN_TEST(test_stream_endpoint_accepts_wss_and_ws);
    RUN_TEST(test_stream_endpoint_rejects_ftp);
    RUN_TEST(test_stream_endpoint_rejects_empty_authority);
    RUN_TEST(test_stream_endpoint_rejects_userinfo);
    RUN_TEST(test_find_meta_value_returns_match);
    RUN_TEST(test_parse_meta_full_record);
    RUN_TEST(test_parse_meta_rejects_bad_endpoint);
    RUN_TEST(test_validate_record_accepts_complete);
    RUN_TEST(test_validate_record_rejects_wrong_decryptor_length);
    RUN_TEST(test_runtime_identity_allows_explicitly_disabled_e2ee_but_not_bad_tls);
    return UNITY_END();
}
