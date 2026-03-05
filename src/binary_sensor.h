#pragma once

#include "sensor.h"

namespace PicoHA {

class BinarySensor : public Sensor<bool> {
public:
    BinarySensor(Device & device, const String & identifier,
                 const String & name)
        : Sensor(device, identifier, name) {}

protected:
    std::function<bool()> getter;

    virtual void fire(const bool & new_value) override {
        this->value = new_value;
        get_mqtt().publish(get_state_topic(), value ? "ON" : "OFF");
    }

    virtual String get_platform() const override { return "binary_sensor"; }
};

}  // namespace PicoHA
