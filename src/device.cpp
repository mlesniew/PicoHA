#include <Arduino.h>

#include <PicoSlugify.h>

#include "device.h"
#include "entity.h"

namespace PicoHA {
Device::Device(
    const String & name, const String & manufacturer,
    const String & model, const String & suggested_area) : name(name),
    manufacturer(manufacturer),
    model(model),
    suggested_area(suggested_area),
    last_update(0), update_interval(1000) {
}

JsonDocument Device::get_autodiscovery_json() const {
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

    if (const Device * parent = get_parent_device()) {
        json["via_device"] = parent->get_unique_id();
    }

    json["connections"][0][0] = "mac";
    json["connections"][0][1] = WiFi.macAddress();
    json["connections"][1][0] = "ip";
    json["connections"][1][1] = WiFi.localIP();

    json["sw_version"] = __DATE__ " " __TIME__;

    return json;
}

String Device::get_default_entity_id_prefix() const {
    return PicoSlugify::slugify(name) + "_";
}

void Device::begin() {
    for (Entity * e : entities) {
        e->begin();
    }

}

void Device::tick() {
    if (millis() - last_update < update_interval) {
        return;
    }

    last_update = millis();

    for (Device * d : devices) {
        d->tick();
    }

    for (Entity * e : entities) {
        e->tick();
    }
}

void Device::fire() {
    last_update = millis();

    for (Device * d : devices) {
        d->fire();
    }

    for (Entity * e : entities) {
        e->fire();
    }
}

void Device::autodiscovery() {
    for (Device * d : devices) {
        d->autodiscovery();
    }

    for (Entity * e : entities) {
        e->autodiscovery();
    }
}

void RootDevice::begin() {

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
}

}