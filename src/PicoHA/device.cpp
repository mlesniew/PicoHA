#include "device.h"

#include <Arduino.h>
#include <PicoSlugify.h>

#include "entity.h"

namespace PicoHA {
AbstractDevice::AbstractDevice(const PicoString & name,
                               const PicoString & manufacturer,
                               const PicoString & model,
                               const PicoString & suggested_area)
    : name(name),
      manufacturer(manufacturer),
      model(model),
      suggested_area(suggested_area) {}

AbstractDevice::~AbstractDevice() {
    Entity * entity = entities;
    while (entity) {
        Entity * next = entity->next;
        delete entity;
        entity = next;
    }

    entities = nullptr;
    devices = nullptr;
}

JsonDocument AbstractDevice::get_autodiscovery_json() const {
    JsonDocument json;
    json[F("name")] = name;
    json[F("identifiers")][0] = get_unique_id();

    if (!manufacturer.isEmpty()) {
        json[F("manufacturer")] = manufacturer;
    }

    if (!model.isEmpty()) {
        json[F("model")] = model;
    }

    if (!suggested_area.isEmpty()) {
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

void AbstractDevice::begin() {
    for (ChildDevice * d = devices; d; d = d->next) {
        d->begin();
    }

    for (Entity * e = entities; e; e = e->next) {
        e->begin(*this);
    }
}

void AbstractDevice::tick() {
    for (ChildDevice * d = devices; d; d = d->next) {
        d->tick();
    }

    for (Entity * e = entities; e; e = e->next) {
        e->tick(*this);
    }
}

void AbstractDevice::fire() {
    for (ChildDevice * d = devices; d; d = d->next) {
        d->fire();
    }

    for (Entity * e = entities; e; e = e->next) {
        e->fire(*this);
    }
}

void AbstractDevice::end() {
    for (Entity * e = entities; e; e = e->next) {
        e->end(*this);
    }

    for (ChildDevice * d = devices; d; d = d->next) {
        d->end();
    }
}

void AbstractDevice::autodiscovery() {
    for (ChildDevice * d = devices; d; d = d->next) {
        d->autodiscovery();
    }

    for (Entity * e = entities; e; e = e->next) {
        e->autodiscovery(*this);
    }
}

Device::~Device() { end(); }

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
    if (!mqtt.connected()) {
        return;
    }

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

void Device::fire() { AbstractDevice::fire(); }

void Device::end() { AbstractDevice::end(); }

}  // namespace PicoHA