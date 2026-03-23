#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PicoMQTT.h>
#include <PicoSlugify.h>

#include "json.h"
#include "utils.h"

namespace PicoHA {

class Entity;
class BinarySensor;
class Event;
class Button;
class Switch;
class Text;
class Select;
class Climate;
template <typename T, String (*to_string)(const T)>
class Sensor;
template <typename T>
class NumericSensor;
template <typename T>
class Number;

class ChildDevice;

class AbstractDevice {
public:
    AbstractDevice(const PicoString & name, const PicoString & manufacturer,
                   const PicoString & model, const PicoString & suggested_area);

    virtual ~AbstractDevice();

    AbstractDevice(const AbstractDevice &) = delete;
    AbstractDevice & operator=(const AbstractDevice &) = delete;

    AbstractDevice(AbstractDevice &&) = delete;
    AbstractDevice & operator=(AbstractDevice &&) = delete;

    virtual JsonDocument get_autodiscovery_json() const;
    virtual PicoJson print_autodiscovery_json(PicoJson && e) const;

    virtual String get_unique_id() const = 0;
    virtual String get_topic_prefix() const = 0;

    virtual String get_availability_topic() const = 0;
    virtual const AbstractDevice * get_parent_device() const = 0;

    PicoString name;
    PicoString manufacturer;
    PicoString model;
    PicoString suggested_area;

    template <typename T>
    T & addEntity(const PicoString & entity_id,
                  const PicoString & entity_name = "") {
        T * e = new T(entity_id, entity_name);
        e->next = entities;
        entities = e;
        return *e;
    }

    BinarySensor & addBinarySensor(const PicoString & id,
                                   const PicoString & name = "");
    Event & addEvent(const PicoString & id, const PicoString & name = "");
    Button & addButton(const PicoString & id, const PicoString & name = "");
    Switch & addSwitch(const PicoString & id, const PicoString & name = "");
    Text & addText(const PicoString & id, const PicoString & name = "");
    Select & addSelect(const PicoString & id, const PicoString & name = "");
    Climate & addClimate(const PicoString & id, const PicoString & name = "");

    template <typename T>
    Sensor<T, to_string_default<T>> & addSensor(const PicoString & id,
                                                const PicoString & name = "");
    template <typename T>
    NumericSensor<T> & addNumericSensor(const PicoString & id,
                                        const PicoString & name = "");
    template <typename T>
    Number<T> & addNumber(const PicoString & id, const PicoString & name = "");

    ChildDevice & addChildDevice(const PicoString & id,
                                 const PicoString & name = "",
                                 const PicoString & manufacturer = "",
                                 const PicoString & model = "",
                                 const PicoString & suggested_area = "");

    String get_default_entity_id_prefix() const {
        return PicoSlugify::slugify(name);
    }

    virtual PicoMQTT::Client & get_mqtt() = 0;

protected:
    virtual void begin();
    virtual void tick();
    virtual void fire();
    virtual void end();
    void autodiscovery();

    ChildDevice * devices = nullptr;
    Entity * entities = nullptr;
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
    virtual PicoJson print_autodiscovery_json(PicoJson && e) const override;

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
    virtual ~ChildDevice() = default;

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

private:
    ChildDevice(AbstractDevice & parent, const PicoString & identifier,
                const PicoString & name, const PicoString & manufacturer,
                const PicoString & model, const PicoString & suggested_area)
        : AbstractDevice(name, manufacturer, model, suggested_area),
          parent(parent),
          identifier(smart_slugify(identifier, '-')) {}

    ChildDevice * next = nullptr;

    friend class AbstractDevice;
};

inline ChildDevice & AbstractDevice::addChildDevice(
    const PicoString & id, const PicoString & name,
    const PicoString & manufacturer, const PicoString & model,
    const PicoString & suggested_area) {
    ChildDevice * d =
        new ChildDevice(*this, id, name, manufacturer, model, suggested_area);
    d->next = devices;
    devices = d;
    return *d;
}

}  // namespace PicoHA