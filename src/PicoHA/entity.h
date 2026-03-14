#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PicoMQTT.h>

#include "device.h"

namespace PicoHA {

class AbstractDevice;

class Entity {
public:
    Entity(AbstractDevice & device, const String & identifier,
           const String & name);
    virtual ~Entity();

    Entity(const Entity &) = delete;
    Entity & operator=(const Entity &) = delete;

    Entity(Entity &&) = delete;
    Entity & operator=(Entity &&) = delete;

    virtual void begin() {}
    virtual void tick() {}
    virtual void fire() {}

    virtual JsonDocument get_autodiscovery_json() const;

    String get_unique_id() const;

    void autodiscovery();

    AbstractDevice & device;
    const String identifier;
    String name;
    String icon;
    String device_class;
    bool is_diagnostic;
    bool enabled_by_default;

protected:
    PicoMQTT::Client & get_mqtt() const { return device.get_mqtt(); }
    virtual String get_platform() const = 0;

    String get_topic_prefix() const {
        return device.get_topic_prefix() + F("/") + identifier;
    }

    virtual String get_state_topic() const { return F(""); }
    virtual String get_command_topic() const { return F(""); }

private:
    String get_autodiscovery_topic() const;
};

class EntityWithCommand : virtual public Entity {
public:
    using Entity::Entity;
    virtual void begin() override;

protected:
    virtual String get_command_topic() const override {
        return get_topic_prefix() + F("/set");
    }
    virtual void on_command(const String & command) = 0;
};

template <typename T>
class EntityWithState : virtual public Entity {
public:
    EntityWithState(AbstractDevice & device, const String & identifier,
                    const String & name)
        : Entity(device, identifier, name),
          update_interval(250),
          last_update(0) {}

    void tick() override {
        if (!getter) {
            return;
        }

        if (update_interval && (millis() - last_update < update_interval)) {
            return;
        }

        const T new_value = getter();
        if (new_value != value) {
            fire(new_value);
        }
    }

    void fire() override {
        if (getter) {
            fire(getter());
        }
    }

    virtual void bind(const T * value) {
        getter = [value] { return *value; };
    }

    virtual void bind(T * value) { bind((const T *)value); }

    std::function<T()> getter;

    unsigned long update_interval;

protected:
    T value;
    unsigned long last_update;

    String get_state_topic() const {
        return getter ? get_topic_prefix() : F("");
    }

    void fire(const T & new_value) {
        value = new_value;
        publish();
        last_update = millis();
    }

    virtual void publish() const {
        get_mqtt().publish(get_state_topic(), String(value));
    }
};

}  // namespace PicoHA