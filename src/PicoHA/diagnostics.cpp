#include "diagnostics.h"

#include "event.h"
#include "input.h"
#include "sensor.h"

namespace {

#if defined(ESP8266)
String reset_reason_to_string(const rst_reason reason) {
    switch (reason) {
        case REASON_DEFAULT_RST:
            return F("power on");
        case REASON_WDT_RST:
            return F("watchdog");
        case REASON_EXCEPTION_RST:
            return F("exception");
        case REASON_SOFT_WDT_RST:
            return F("soft watchdog");
        case REASON_SOFT_RESTART:
            return F("software reset");
        case REASON_DEEP_SLEEP_AWAKE:
            return F("deep sleep wake");
        case REASON_EXT_SYS_RST:
            return F("external reset");
        default:
            return F("unknown");
    };
}
#endif

#ifdef ESP32
String reset_reason_to_string(const esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON:
            return "power_on";
        case ESP_RST_EXT:
            return "external_reset";
        case ESP_RST_SW:
            return "software_reset";
        case ESP_RST_PANIC:
            return "panic";
        case ESP_RST_INT_WDT:
            return "interrupt_watchdog";
        case ESP_RST_TASK_WDT:
            return "task_watchdog";
        case ESP_RST_WDT:
            return "watchdog";
        case ESP_RST_DEEPSLEEP:
            return "wake_from_deep_sleep";
        case ESP_RST_BROWNOUT:
            return "brownout";
        case ESP_RST_SDIO:
            return "sdio_reset";
        default:
            return "unknown";
    }
}
#endif

}  // namespace

namespace PicoHA {

void add_diagnostic_entities(Device & device) {
    auto & reboot_event = device.addEntity<Event>(F("reboot"), F("Reboot"));
    reboot_event.icon = F("restart");
    reboot_event.trigger();
    reboot_event.is_diagnostic = true;

    auto & reset_button = device.addEntity<Button>(F("reset"), F("Reset"));
    reset_button.icon = F("restart");
    reset_button.on_press = [] { ESP.restart(); };
    reset_button.is_diagnostic = true;

    auto & free_heap_sensor =
        device.addEntity<NumericSensor<int>>(F("free_heap"), F("Free Heap"));
    free_heap_sensor.icon = F("memory");
    free_heap_sensor.getter =
#if defined(ESP8266)
        ESP.getFreeHeap;
#else
        esp_get_free_heap_size;
#endif
    free_heap_sensor.unit_of_measurement = F("B");
    free_heap_sensor.device_class = F("data_size");
    free_heap_sensor.is_diagnostic = true;
    free_heap_sensor.update_interval = 60000;

    auto & min_free_heap_sensor = device.addEntity<NumericSensor<int>>(
        F("min_free_heap"), F("Min Free Heap"));
    min_free_heap_sensor.icon = F("memory");
    min_free_heap_sensor.getter =
#if defined(ESP8266)
        [] {
            static uint32_t ret = ESP.getFreeHeap();
            if (ESP.getFreeHeap() < ret) {
                ret = ESP.getFreeHeap();
            }
            return ret;
        };
#elif defined(ESP32)
        esp_get_minimum_free_heap_size;
#endif
    min_free_heap_sensor.unit_of_measurement = F("B");
    min_free_heap_sensor.device_class = F("data_size");
    min_free_heap_sensor.is_diagnostic = true;
    min_free_heap_sensor.update_interval = 1000;

#ifdef ESP8266
    auto & max_free_block_sensor = device.addEntity<NumericSensor<int>>(
        F("max_free_block"), F("Max Free Block"));
    max_free_block_sensor.icon = F("memory");
    max_free_block_sensor.getter = [] { return ESP.getMaxFreeBlockSize(); };
    max_free_block_sensor.unit_of_measurement = F("B");
    max_free_block_sensor.device_class = F("data_size");
    max_free_block_sensor.is_diagnostic = true;
    max_free_block_sensor.update_interval = 60000;
#endif

    auto & rssi_sensor =
        device.addEntity<NumericSensor<int>>(F("rssi"), F("WiFi RSSI"));
    rssi_sensor.icon = F("signal");
    rssi_sensor.getter = [] { return WiFi.RSSI(); };
    rssi_sensor.unit_of_measurement = F("dBm");
    rssi_sensor.device_class = F("signal_strength");
    rssi_sensor.is_diagnostic = true;
    rssi_sensor.update_interval = 60000;

    auto & ssid_sensor =
        device.addEntity<Sensor<String>>(F("wifi_ssid"), F("WiFi SSID"));
    ssid_sensor.icon = F("wifi");
    ssid_sensor.getter = [] { return WiFi.SSID(); };
    ssid_sensor.is_diagnostic = true;

    auto & wifi_channel_sensor = device.addEntity<NumericSensor<unsigned char>>(
        F("wifi_channel"), F("WiFi Channel"));
    wifi_channel_sensor.icon = F("wifi");
    wifi_channel_sensor.getter = [] { return WiFi.channel(); };
    wifi_channel_sensor.is_diagnostic = true;

#ifdef ESP8266
    auto & reset_reason_sensor =
        device.addEntity<EnumSensor<rst_reason, reset_reason_to_string>>(
            F("reset_reason"), F("Reset Reason"));
    reset_reason_sensor.icon = F("timeline-question");
    reset_reason_sensor.getter = [] {
        rst_info * info = ESP.getResetInfoPtr();
        return (rst_reason)info->reason;
    };
    reset_reason_sensor.is_diagnostic = true;
#endif

#ifdef ESP32
    auto & reset_reason_sensor =
        device
            .addEntity<EnumSensor<esp_reset_reason_t, reset_reason_to_string>>(
                F("reset_reason"), F("Reset Reason"));
    reset_reason_sensor.icon = F("timeline-question");
    reset_reason_sensor.getter = [] { return esp_reset_reason(); };
    reset_reason_sensor.is_diagnostic = true;
#endif
}

}  // namespace PicoHA
