// Pure ESP-IDF startup. An app-local keys.h is passed through the runtime API;
// it never needs to be on the library component's private include path.
#include <stdint.h>
#include <stdio.h>
#include <nvs_flash.h>

// Arduino-generated keys use PROGMEM; const data already lives in ESP flash.
#ifndef PROGMEM
#define PROGMEM
#endif
#if __has_include("keys.h")
#include "keys.h"
#endif
#include "device_app.h"

extern "C" void app_main(void)
{
    // Never erase persistent credentials/rollback state to hide an init failure.
    // Recovery or factory reset is an explicit operator action.
    ESP_ERROR_CHECK(nvs_flash_init());
    static Firmngin device(nullptr, nullptr); // Without runtime keys, load a provisioned flash identity.
#if defined(USE_CA_CERT)
    FirmnginIdentityRecord identity;
    identity.deviceId = DEVICE_ID;
    identity.deviceKey = DEVICE_KEY;
    identity.decryptor = DECRYPTOR;
    identity.tlsMode = "ca";
    identity.caCert = CA_CERT;
    identity.clientCert = CLIENT_CERT;
    identity.privateKey = PRIVATE_KEY;
#if defined(HAS_SERVICE_CA_CERT)
    identity.serviceCaCert = SERVICE_CA_CERT;
#else
    identity.serviceCaCert = CA_CERT; // Only suitable when the API shares the broker's CA.
#endif
    identity.brokerAddr = FIRMNGIN_BROKER_ADDR;
    identity.brokerPort = FIRMNGIN_BROKER_PORT;
    identity.apiBaseUrl = FIRMNGIN_API_BASE_URL;
    identity.otaBaseUrl = FIRMNGIN_OTA_BASE_URL;
    if (!device.setIdentity(identity))
    {
        printf("Invalid runtime identity; check CA-mode keys.h\n");
        return;
    }
#endif
#if defined(FIRMNGIN_HAS_OTA_FIRMWARE_PUBLIC_KEY)
    device.setOTAFirmwarePublicKey(OTA_FIRMWARE_PUBLIC_KEY);
#endif
    run_device(device, "YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");
}
