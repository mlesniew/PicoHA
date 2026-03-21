#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PicoMQTT.h>
#include <PicoSlugify.h>

#include <list>

#include "utils.h"

namespace PicoHA {

class Entity;

class ChildDevice;

class AbstractDevice {
public:
    AbstractDevice(const PicoString & name, const PicoString & manufacturer,
                   const PicoString & model, const PicoString & suggested_area);

    virtual ~AbstractDevice() {}

    AbstractDevice(const AbstractDevice &) = delete;
    AbstractDevice & operator=(const AbstractDevice &) = delete;

    AbstractDevice(AbstractDevice &&) = delete;
    AbstractDevice & operator=(AbstractDevice &&) = delete;

    virtual JsonDocument get_autodiscovery_json() const;

    virtual String get_unique_id() const = 0;
    virtual String get_topic_prefix() const = 0;

    virtual String get_availability_topic() const = 0;
    virtual const AbstractDevice * get_parent_device() const = 0;

    const PicoString identifier;
    PicoString name;
    PicoString manufacturer;
    PicoString model;
    PicoString suggested_area;

    friend class Entity;
    friend class ChildDevice;

    virtual PicoMQTT::Client & get_mqtt() = 0;

protected:
    virtual void begin();
    virtual void tick();
    virtual void fire();
    virtual void end();
    void autodiscovery();

    String get_default_entity_id_prefix() const {
        return PicoSlugify::slugify(name);
    }

    std::set<ChildDevice *> devices;
    std::set<Entity *> entities;
};

class Device : public AbstractDevice {
public:
    Device(PicoMQTT::Client & mqtt, const String & name,
           const String & manufacturer, const String & model,
           const String & suggested_area = "")
        : AbstractDevice(name, manufacturer, model, suggested_area),
          mqtt(mqtt),
          last_autodiscovery_time(0) {}

    virtual ~Device();

    virtual JsonDocument get_autodiscovery_json() const override;

    virtual void begin() override;
    virtual void tick() override;
    virtual void fire() override;
    virtual void end() override;

    String get_board_id() const {
#ifdef ESP32
        return String((uint32_t)(ESP.getEfuseMac() >> 24), HEX);
#elif defined(ESP8266)
        return String(ESP.getChipId(), HEX);
#else
#error "Unsupported platform"
#endif
    }

    virtual String get_unique_id() const {
        return PicoSlugify::slugify(model, '-') + "-" + get_board_id();
    }

    virtual String get_topic_prefix() const override {
        return PicoSlugify::slugify(model, '-') + "/" + get_board_id();
    }

    virtual const AbstractDevice * get_parent_device() const override {
        return nullptr;
    }

    virtual String get_availability_topic() const override {
        return get_topic_prefix() + F("/availability");
    }

    virtual PicoMQTT::Client & get_mqtt() override { return mqtt; }

protected:
    PicoMQTT::Client & mqtt;
    unsigned long last_autodiscovery_time;
};

class ChildDevice : public AbstractDevice {
public:
    ChildDevice(AbstractDevice & parent, const PicoString & identifier,
                const PicoString & name, const PicoString & manufacturer,
                const PicoString & model, const PicoString & suggested_area)
        : AbstractDevice(name, manufacturer, model, suggested_area),
          parent(parent),
          identifier(smart_slugify(identifier, '-')) {
        parent.devices.insert(this);
    }

    virtual ~ChildDevice() {
        end();
        parent.devices.erase(this);
    }

    virtual String get_unique_id() const {
        return parent.get_unique_id() + F("-") + identifier;
    }

    virtual String get_topic_prefix() const {
        return parent.get_topic_prefix() + F("/") + identifier;
    }

    virtual const AbstractDevice * get_parent_device() const override {
        return &parent;
    }

    virtual String get_availability_topic() const {
        return parent.get_availability_topic();
    }

    virtual PicoMQTT::Client & get_mqtt() override { return parent.get_mqtt(); }

    AbstractDevice & parent;
    const PicoString identifier;
};

}  // namespace PicoHA