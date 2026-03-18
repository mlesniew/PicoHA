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
    QueuedEvent * reboot_event =
        new QueuedEvent(device, F("reboot"), F("Reboot"));
    reboot_event->icon = F("restart");
    reboot_event->trigger();
    reboot_event->is_diagnostic = true;

    Button * reset_button = new Button(device, F("reset"), F("Reset"));
    reset_button->icon = F("restart");
    reset_button->on_press = [] { ESP.restart(); };
    reset_button->is_diagnostic = true;

    NumericSensor<int> * free_heap_sensor =
        new NumericSensor<int>(device, F("free_heap"), F("Free Heap"));
    free_heap_sensor->icon = F("memory");
    free_heap_sensor->getter = [] { return ESP.getFreeHeap(); };
    free_heap_sensor->unit_of_measurement = F("B");
    free_heap_sensor->device_class = F("data_size");
    free_heap_sensor->is_diagnostic = true;
    free_heap_sensor->update_interval = 60000;

    NumericSensor<int> * min_free_heap_sensor =
        new NumericSensor<int>(device, F("min_free_heap"), F("Min Free Heap"));
    min_free_heap_sensor->icon = F("memory");
    min_free_heap_sensor->getter = [] {
        static uint32_t ret = ESP.getFreeHeap();
        if (ESP.getFreeHeap() < ret) {
            ret = ESP.getFreeHeap();
        }
        return ret;
    };
    min_free_heap_sensor->unit_of_measurement = F("B");
    min_free_heap_sensor->device_class = F("data_size");
    min_free_heap_sensor->is_diagnostic = true;
    min_free_heap_sensor->update_interval = 1000;

    NumericSensor<int> * max_free_block_sensor = new NumericSensor<int>(
        device, F("max_free_block"), F("Max Free Block"));
    max_free_block_sensor->icon = F("memory");
    max_free_block_sensor->getter = [] { return ESP.getMaxFreeBlockSize(); };
    max_free_block_sensor->unit_of_measurement = F("B");
    max_free_block_sensor->device_class = F("data_size");
    max_free_block_sensor->is_diagnostic = true;
    max_free_block_sensor->update_interval = 60000;

    NumericSensor<int> * rssi_sensor =
        new NumericSensor<int>(device, F("rssi"), F("WiFi RSSI"));
    rssi_sensor->icon = F("signal");
    rssi_sensor->getter = [] { return WiFi.RSSI(); };
    rssi_sensor->unit_of_measurement = F("dBm");
    rssi_sensor->device_class = F("signal_strength");
    rssi_sensor->is_diagnostic = true;
    rssi_sensor->update_interval = 60000;

    Sensor<String> * ssid_sensor =
        new Sensor<String>(device, F("wifi_ssid"), F("WiFi SSID"));
    ssid_sensor->icon = F("wifi");
    ssid_sensor->getter = [] { return WiFi.SSID(); };
    ssid_sensor->is_diagnostic = true;

    NumericSensor<unsigned char> * wifi_channel_sensor =
        new NumericSensor<unsigned char>(device, F("wifi_channel"),
                                         F("WiFi Channel"));
    wifi_channel_sensor->icon = F("wifi");
    wifi_channel_sensor->getter = [] { return WiFi.channel(); };
    wifi_channel_sensor->is_diagnostic = true;

#ifdef ESP8266
    EnumSensor<rst_reason, reset_reason_to_string> * reset_reason_sensor =
        new EnumSensor<rst_reason, reset_reason_to_string>(
            device, F("reset_reason"), F("Reset Reason"));
    reset_reason_sensor->icon = F("timeline-question");
    reset_reason_sensor->getter = [] {
        rst_info * info = ESP.getResetInfoPtr();
        return (rst_reason)info->reason;
    };
    reset_reason_sensor->is_diagnostic = true;
#endif

#ifdef ESP32
    EnumSensor<esp_reset_reason_t, reset_reason_to_string> *
        reset_reason_sensor =
            new EnumSensor<esp_reset_reason_t, reset_reason_to_string>(
                device, F("reset_reason"), F("Reset Reason"));
    reset_reason_sensor->icon = F("timeline-question");
    reset_reason_sensor->getter = [] { return ESP.getResetReason(); };
    reset_reason_sensor->is_diagnostic = true;
#endif
}

}  // namespace PicoHA
