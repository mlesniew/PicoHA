#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PicoMQTT.h>
#include <PicoSlugify.h>

#include <list>

namespace PicoHA {

class Entity;

class AbstractDevice {
public:
    AbstractDevice(const String & name, const String & manufacturer,
                   const String & model, const String & suggested_area);

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

    virtual String get_default_entity_id_prefix() const;

    virtual void begin();
    virtual void tick();
    virtual void fire();
    void autodiscovery();

    const String identifier;
    String name;
    String manufacturer;
    String model;
    String suggested_area;

    friend class Entity;
    friend class ChildDevice;

    virtual PicoMQTT::Client & get_mqtt() = 0;

protected:
    std::set<AbstractDevice *> devices;
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

    virtual JsonDocument get_autodiscovery_json() const override;

    virtual void tick() override;

    void add_diagnostic_entities();

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
        return get_topic_prefix() + "/availability";
    }

    virtual void begin() override;

    virtual PicoMQTT::Client & get_mqtt() override { return mqtt; }

protected:
    PicoMQTT::Client & mqtt;
    unsigned long last_autodiscovery_time;
};

class ChildDevice : public AbstractDevice {
public:
    ChildDevice(AbstractDevice & parent, const String & identifier,
                const String & name, const String & manufacturer,
                const String & model, const String & suggested_area)
        : AbstractDevice(name, manufacturer, model, suggested_area),
          parent(parent),
          identifier(PicoSlugify::slugify(identifier, '-')) {
        parent.devices.insert(this);
    }

    virtual ~ChildDevice() { parent.devices.erase(this); }

    virtual String get_unique_id() const {
        return parent.get_unique_id() + "-" + identifier;
    }

    virtual String get_topic_prefix() const {
        return parent.get_topic_prefix() + "/" + identifier;
    }

    virtual const AbstractDevice * get_parent_device() const override {
        return &parent;
    }

    virtual String get_availability_topic() const {
        return parent.get_availability_topic();
    }

    virtual PicoMQTT::Client & get_mqtt() override { return parent.get_mqtt(); }

    AbstractDevice & parent;
    const String identifier;
};

}  // namespace PicoHA