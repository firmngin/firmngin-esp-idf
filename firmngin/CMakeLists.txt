# Firmngin as an ESP-IDF component.
#
# The core sources live in ../src (the public headers sit next to the
# implementation so quoted includes between core files resolve unchanged).
# This wrapper gives them a component identity named "firmngin" so IDF projects
# can `REQUIRES firmngin`.
#
# The vendored esp_websocket_client copy is NOT part of the core: pure IDF pulls
# the registry component declared in idf_component.yml, and the Arduino port
# (firmnginkit-library) keeps its own copy for Arduino core 3.x, which stopped
# bundling the component.
#
# Sources are listed explicitly instead of globbed: a new core file that is not
# registered here fails the IDF build loudly instead of silently vanishing.

set(FNG_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/../src")

idf_component_register(
    SRCS
        "${FNG_SRC_DIR}/audio_stream_protocol.cpp"
        "${FNG_SRC_DIR}/crypto.cpp"
        "${FNG_SRC_DIR}/firmngin.cpp"
        "${FNG_SRC_DIR}/firmngin_gateway.cpp"
        "${FNG_SRC_DIR}/firmngin_i2s.cpp"
        "${FNG_SRC_DIR}/firmngin_identity.cpp"
        "${FNG_SRC_DIR}/firmngin_mqtt.cpp"
        "${FNG_SRC_DIR}/hmac.cpp"
        "${FNG_SRC_DIR}/md5.cpp"
        "${FNG_SRC_DIR}/ota.cpp"
        "${FNG_SRC_DIR}/ota_progress.cpp"
        "${FNG_SRC_DIR}/ota_rollback.cpp"
        "${FNG_SRC_DIR}/ota_transfer.cpp"
        "${FNG_SRC_DIR}/queue.cpp"
        "${FNG_SRC_DIR}/queue_format.cpp"
    INCLUDE_DIRS "${FNG_SRC_DIR}"
    REQUIRES
        esp_http_client app_update nvs_flash esp_timer mbedtls mqtt driver freertos json
    PRIV_REQUIRES
        esp-tls tcp_transport esp_partition spi_flash esp_netif lwip esp_event
)
