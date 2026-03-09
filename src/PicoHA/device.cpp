#include "device.h"

#include <Arduino.h>
#include <PicoSlugify.h>

#include "entity.h"

namespace PicoHA {
AbstractDevice::AbstractDevice(const String & name, const String & manufacturer,
                               const String & model,
                               const String & suggested_area)
    : name(name),
      manufacturer(manufacturer),
      model(model),
      suggested_area(suggested_area),
      last_update(0),
      update_interval(250) {}

JsonDocument AbstractDevice::get_autodiscovery_json() const {
    JsonDocument json;
    json["name"] = name;
    json["identifiers"][0] = get_unique_id();

    if (manufacturer.length()) {
        json["manufacturer"] = manufacturer;
    }

    if (model.length()) {
        json["model"] = model;
    }

    if (suggested_area.length()) {
        json["suggested_area"] = suggested_area;
    }

    if (const AbstractDevice * parent = get_parent_device()) {
        json["via_device"] = parent->get_unique_id();
    }

    json["connections"][0][0] = "mac";
    json["connections"][0][1] = WiFi.macAddress();
    json["connections"][1][0] = "ip";
    json["connections"][1][1] = WiFi.localIP();

    json["sw_version"] = __DATE__ " " __TIME__;

    return json;
}

JsonDocument Device::get_autodiscovery_json() const {
    JsonDocument json = AbstractDevice::get_autodiscovery_json();

    json["connections"][0][0] = "mac";
    json["connections"][0][1] = WiFi.macAddress();
    json["connections"][1][0] = "ip";
    json["connections"][1][1] = WiFi.localIP();

    json["sw_version"] = __DATE__ " " __TIME__;

    return json;
}

String AbstractDevice::get_default_entity_id_prefix() const {
    return PicoSlugify::slugify(name) + "_";
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
    if (millis() - last_update < update_interval) {
        return;
    }

    last_update = millis();

    for (AbstractDevice * d : devices) {
        d->tick();
    }

    for (Entity * e : entities) {
        e->tick();
    }
}

void AbstractDevice::fire() {
    last_update = millis();

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
    mqtt.will.payload = "offline";
    mqtt.will.retain = true;

    mqtt.connected_callback = [this] {
        // send autodiscovery messages
        autodiscovery();

        // notify about availability
        mqtt.publish(get_availability_topic(), "online", 0, true);

        // publish diagnostics right away
        fire();
    };

    AbstractDevice::begin();
}

}  // namespace PicoHA