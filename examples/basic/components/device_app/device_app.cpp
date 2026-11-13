#include "device_app.h"
#include <atomic>
#include <stdio.h>
#include <string.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>

namespace
{
std::atomic<bool> connected{false};

void wifi_event(void *, esp_event_base_t base, int32_t id, void *)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
        esp_wifi_connect();
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    {
        connected = false;
        esp_wifi_connect();
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
        connected = true;
}
}

void run_device(Firmngin &device, const char *ssid, const char *password)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, wifi_event, nullptr));
    wifi_config_t wifi = {};
    strncpy(reinterpret_cast<char *>(wifi.sta.ssid), ssid, sizeof(wifi.sta.ssid) - 1);
    strncpy(reinterpret_cast<char *>(wifi.sta.password), password, sizeof(wifi.sta.password) - 1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
    ESP_ERROR_CHECK(esp_wifi_start());

    ON_ENTITY(Entity("2"), [](EntityCommand &cmd) {
        printf("entity cmd: key=%s value=%s\n", cmd.key().c_str(), cmd.value().c_str());
    });
    while (!connected) vTaskDelay(pdMS_TO_TICKS(200));
    device.linkUp(true);
    device.begin();
    int counter = 0;
    while (true)
    {
        // Only loop's task touches Firmngin. The WiFi task shares one atomic flag.
        device.linkUp(connected.load());
        device.loop();
        vTaskDelay(pdMS_TO_TICKS(10));
        if (++counter % 1000 == 0) device.pushEntity("temperature", 25.0);
    }
}
