#include "device.h"

#include <Arduino.h>
#include <PicoSlugify.h>

#include "entity.h"
#include "event.h"
#include "input.h"
#include "numeric_sensor.h"

namespace PicoHA {
AbstractDevice::AbstractDevice(const String & name, const String & manufacturer,
                               const String & model,
                               const String & suggested_area)
    : name(name),
      manufacturer(manufacturer),
      model(model),
      suggested_area(suggested_area) {}

JsonDocument AbstractDevice::get_autodiscovery_json() const {
    JsonDocument json;
    json[F("name")] = name;
    json[F("identifiers")][0] = get_unique_id();

    if (manufacturer.length()) {
        json[F("manufacturer")] = manufacturer;
    }

    if (model.length()) {
        json[F("model")] = model;
    }

    if (suggested_area.length()) {
        json[F("suggested_area")] = suggested_area;
    }

    if (const AbstractDevice * parent = get_parent_device()) {
        json[F("via_device")] = parent->get_unique_id();
    }

    return json;
}

JsonDocument Device::get_autodiscovery_json() const {
    JsonDocument json = AbstractDevice::get_autodiscovery_json();

    json[F("connections")][0][0] = F("mac");
    json[F("connections")][0][1] = WiFi.macAddress();
    json[F("connections")][1][0] = F("ip");
    json[F("connections")][1][1] = WiFi.localIP().toString();

    json[F("sw_version")] = __DATE__ " " __TIME__;

    return json;
}

void Device::add_diagnostic_entities() {
    QueuedEvent * reboot_event =
        new QueuedEvent(*this, F("reboot"), F("Reboot"));
    reboot_event->icon = F("restart");
    reboot_event->trigger();
    reboot_event->is_diagnostic = true;

    Button * reset_button = new Button(*this, F("reset"), F("Reset"));
    reset_button->icon = F("restart");
    reset_button->on_press = [] { ESP.restart(); };
    reset_button->is_diagnostic = true;

    NumericSensor<int> * free_heap_sensor =
        new NumericSensor<int>(*this, F("free_heap"), F("Free Heap"));
    free_heap_sensor->icon = F("memory");
    free_heap_sensor->getter = [] { return ESP.getFreeHeap(); };
    free_heap_sensor->unit_of_measurement = F("B");
    free_heap_sensor->device_class = F("data_size");
    free_heap_sensor->is_diagnostic = true;
    free_heap_sensor->update_interval = 60000;

    NumericSensor<int> * rssi_sensor =
        new NumericSensor<int>(*this, F("rssi"), F("WiFi RSSI"));
    rssi_sensor->icon = F("signal");
    rssi_sensor->getter = [] { return WiFi.RSSI(); };
    rssi_sensor->unit_of_measurement = F("dBm");
    rssi_sensor->device_class = F("signal_strength");
    rssi_sensor->is_diagnostic = true;
    rssi_sensor->update_interval = 60000;

    Sensor<String> * ssid_sensor =
        new Sensor<String>(*this, F("wifi_ssid"), F("WiFi SSID"));
    ssid_sensor->icon = F("wifi");
    ssid_sensor->getter = [] { return WiFi.SSID(); };
    ssid_sensor->is_diagnostic = true;

    NumericSensor<unsigned char> * wifi_channel_sensor =
        new NumericSensor<unsigned char>(*this, F("wifi_channel"),
                                         F("WiFi Channel"));
    wifi_channel_sensor->icon = F("wifi");
    wifi_channel_sensor->getter = [] { return WiFi.channel(); };
    wifi_channel_sensor->is_diagnostic = true;

    Sensor<String> * reset_reason_sensor =
        new Sensor<String>(*this, F("reset_reason"), F("Reset Reason"));
    reset_reason_sensor->icon = F("timeline-question");
#if defined(ESP8266)
    reset_reason_sensor->getter = [] {
        rst_info * info = ESP.getResetInfoPtr();

        switch (info->reason) {
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
    };
#elif defined(ESP32)
    reset_reason_sensor->getter = [] {
        esp_reset_reason_t reason = esp_reset_reason();

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
    };
#else
#error "Unsupported platform"
#endif
    reset_reason_sensor->is_diagnostic = true;
}

String AbstractDevice::get_default_entity_id_prefix() const {
    return PicoSlugify::slugify(name);
}

void AbstractDevice::begin() {
    for (AbstractDevice * d : devices) {
        d->begin();
    }

    for (Entity * e : entities) {
        e->begin();
    }
}

void AbstractDevice::tick() {
    for (AbstractDevice * d : devices) {
        d->tick();
    }

    for (Entity * e : entities) {
        e->tick();
    }
}

void AbstractDevice::fire() {
    for (AbstractDevice * d : devices) {
        d->fire();
    }

    for (Entity * e : entities) {
        e->fire();
    }
}

void AbstractDevice::autodiscovery() {
    for (AbstractDevice * d : devices) {
        d->autodiscovery();
    }

    for (Entity * e : entities) {
        e->autodiscovery();
    }
}

void Device::begin() {
    if (mqtt.client_id.isEmpty()) {
        mqtt.client_id = get_unique_id();
    }

    mqtt.will.topic = get_availability_topic();
    mqtt.will.payload = F("offline");
    mqtt.will.retain = true;

    mqtt.connected_callback = [this] {
        // send autodiscovery messages
        autodiscovery();
        last_autodiscovery_time = millis();

        // notify about availability
        mqtt.publish(get_availability_topic(), F("online"), 0, true);

        // publish right away
        fire();
    };

    AbstractDevice::begin();
}

void Device::tick() {
    if (last_autodiscovery_time &&
        (millis() - last_autodiscovery_time >= 30 * 1000)) {
        // It's been 30 seconds since we last sent autodiscovery messages,
        // Home Assistant should now have discovered us and subscribed to
        // all topics.  Resend all measurements to ensure Home Assistant
        // has the latest values.
        last_autodiscovery_time = 0;
        fire();
    }

    AbstractDevice::tick();
}

}  // namespace PicoHA