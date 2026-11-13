# Firmngin ESP-IDF Component

<p align="center">
  <img src="https://raw.githubusercontent.com/firmngin/firmngin-esp-idf/main/logo.png" alt="Firmngin" width="96">
</p>

IoT library for ESP32 on **pure ESP-IDF**: secure mTLS communication, OTA firmware
updates, image upload, GPS location, and payment/verification flows for the
[Firmngin platform](https://firmngin.dev).

This repository is the **source of truth**. The Arduino library
([Firmngin-Arduino](https://github.com/firmngin/Firmngin-Arduino)) is a port that
mirrors `src/` verbatim — never edit the port's `src/` directly.

## Features

- **ESP-IDF 5.1+**, no Arduino core
- **Event-driven API** using `on()` callbacks: raw or typed
- **Typed objects**: `Verifications`, `Payments`, `Dispenses`, `Usages`, `DeviceStates`, `Inits`, `EntityCommand`
- **Vending mode** with SKU dispense callbacks
- **Image Upload** for camera-equipped devices
- **OTA updates** with bootloader rollback
- **Secure connection** via mTLS

## Layout

```
firmngin/           IDF component wrapper (CMakeLists.txt, idf_component.yml)
src/                the core: public headers sit next to the implementation
examples/basic/     pure IDF example (WiFi via esp_wifi/esp_netif)
examples/keys.h.template
scripts/            IDF purity gate, version sync
test/native/        host unit tests (no ESP_PLATFORM)
```

## Install as a component

```cmake
# project CMakeLists.txt, before project(...) discovers components
list(APPEND EXTRA_COMPONENT_DIRS "<path>/firmngin-esp-idf/firmngin")
```

Then `REQUIRES firmngin` in your own component. The component manifest
(`firmngin/idf_component.yml`) pulls `espressif/esp_websocket_client` and
`joltwallet/littlefs`.

## Example

```sh
cd examples/basic
python -m pip install 'idf-component-manager>=1.5.2,<2'
idf.py set-target esp32
idf.py build
```

or with PlatformIO: `pio run -d examples/basic -e espidf`.

`main/keys.h` is app-local. Select `USE_CA_CERT` and pass its values through
`setIdentity(FirmnginIdentityRecord)` **before** `begin()`; the library copies the
record, and malformed keys are rejected. Without CA-mode keys the example loads a
previously provisioned flash identity. It never erases NVS on an initialization
error — storage recovery or factory reset is an explicit operator action.

Application code activates WiFi with `esp_wifi`/`esp_netif` and reports link state
through `fngin.linkUp(bool)`, which keeps the network seam framework-neutral.
Verified HTTPS is the default; `setInsecure(true)` requires an SDK built with
`CONFIG_ESP_TLS_INSECURE` and `CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY`.

## Compatibility with Arduino

`src/fng_compat.h` is the single framework seam: under `ARDUINO` it aliases the
Arduino `String`, otherwise the core runs on a `std::string`-backed `FString`.
`scripts/check_idf_purity.sh` fails the build when Arduino-only APIs leak outside
that layer or an `#if defined(ARDUINO)` fence.

Arduino-ESP32 core 3.x stopped bundling `esp_websocket_client`; the port carries a
vendored copy (`src/esp_websocket_client_vendored.*`) that this repository does
**not** contain. Those two files plus `keys.h.template` are the only files that
differ between the two repositories.

## Versioning

One `VERSION` file drives the component manifest and `src/firmngin_version.h`:

```sh
python3 scripts/sync_version.py          # write
python3 scripts/sync_version.py --check  # CI gate
```

The Arduino port releases under the same version number, so a PlatformIO registry
release always maps to a core tag.

## Test

```sh
bash scripts/check_idf_purity.sh
cd test/native && pio test -e native --without-uploading   # 82 cases
```

Verified: IDF 5.1 and 5.3 builds of `examples/basic`, 82 native cases. OTA
end-to-end, provisioning, and audio on real hardware are not yet tested.
