#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PicoMQTT.h>

#include <functional>

#include "device.h"
#include "json.h"
#include "utils.h"

namespace PicoHA {

class Entity {
public:
    Entity(const PicoString & identifier, const PicoString & name);
    virtual ~Entity() {}

    Entity(const Entity &) = delete;
    Entity & operator=(const Entity &) = delete;

    Entity(Entity &&) = delete;
    Entity & operator=(Entity &&) = delete;

    virtual JsonDocument get_autodiscovery_json(
        const AbstractDevice & device) const;

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const;

    String get_unique_id(const AbstractDevice & device) const;

    void autodiscovery(AbstractDevice & device);

    const PicoString identifier;
    PicoString name;
    PicoString icon;
    PicoString device_class;
    bool is_diagnostic;
    bool enabled_by_default;

    friend class AbstractDevice;

protected:
    virtual void begin(AbstractDevice & device) {}
    virtual void tick(AbstractDevice & device) {}
    virtual void fire(AbstractDevice & device) {}
    virtual void end(AbstractDevice & device) {}

    PicoMQTT::Client & get_mqtt(AbstractDevice & device) const {
        return device.get_mqtt();
    }
    virtual String get_platform() const = 0;

    String get_topic_prefix(const AbstractDevice & device) const {
        return device.get_topic_prefix() + F("/") + identifier;
    }

    virtual String get_state_topic(const AbstractDevice & device) const {
        return F("");
    }
    virtual String get_command_topic(const AbstractDevice & device) const {
        return F("");
    }

private:
    String get_autodiscovery_topic(const AbstractDevice & device) const;

    Entity * next;
};

class EntityWithCommand : virtual public Entity {
public:
    using Entity::Entity;
    virtual ~EntityWithCommand();

protected:
    virtual void begin(AbstractDevice & device) override;
    virtual void end(AbstractDevice & device) override;
    virtual String get_command_topic(
        const AbstractDevice & device) const override {
        return get_topic_prefix(device) + F("/set");
    }
    virtual void on_command(const String & command) = 0;
};

template <typename T, String (*to_string)(const T) = to_string_default<T>>
class EntityWithState : virtual public Entity {
public:
    EntityWithState(const PicoString & identifier, const PicoString & name)
        : Entity(identifier, name), update_interval(250), last_update(0) {}

protected:
    void begin(AbstractDevice & device) override {
        if (getter) {
            value = getter();
        }
    }

    void tick(AbstractDevice & device) override {
        if (!getter) {
            return;
        }

        if (update_interval && (millis() - last_update < update_interval)) {
            return;
        }

        const T new_value = getter();
        if (new_value != value) {
            fire(device, new_value);
        }
    }

    void fire(AbstractDevice & device) override {
        if (getter) {
            fire(device, getter());
        }
    }

public:
    virtual void bind(const T * value) {
        getter = [value] { return *value; };
    }

    virtual void bind(T * value) { bind((const T *)value); }

    std::function<T()> getter;

    unsigned long update_interval;

protected:
    T value;
    unsigned long last_update;

    String get_state_topic(const AbstractDevice & device) const {
        return getter ? get_topic_prefix(device) : F("");
    }

    void fire(AbstractDevice & device, const T & new_value) {
        value = new_value;
        publish(device);
        last_update = millis();
    }

    void publish(AbstractDevice & device) const {
        const String topic = get_state_topic(device);
        if (!topic.isEmpty()) {
            device.get_mqtt().publish(topic, to_string(value));
        }
    }
};

}  // namespace PicoHA
