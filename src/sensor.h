#pragma once

#include <entity.h>

#include <functional>

namespace PicoHA {

template <typename T>
class Sensor : public Entity {
public:
    Sensor(Device & device, const String & identifier, const String & name = "")
        : Entity(device, identifier, name) {}

    JsonDocument get_autodiscovery_json() const {
        JsonDocument json = Entity::get_autodiscovery_json();
        json["state_topic"] = get_state_topic();
        return json;
    }

    void tick() override {
        const T new_value = get();
        if (new_value != value) {
            fire(new_value);
        }
    }

    void fire() override { fire(get()); }

    void bind(const std::function<T()> & getter) { this->getter = getter; }

    void bind(const T * value) {
        getter = [value] { return *value; };
    }

protected:
    virtual String get_platform() const override { return "sensor"; }
    String get_state_topic() const { return get_topic_prefix(); }

    virtual T get() { return getter ? getter() : T(); }

    virtual void fire(const T & new_value) {
        value = new_value;
        get_mqtt().publish(get_state_topic(), String(value));
    }

    T value;

private:
    std::function<T()> getter;
};

}  // namespace PicoHA