#pragma once

#include "sensor.h"

namespace PicoHA {

class BinarySensor : public Sensor<bool> {
public:
    BinarySensor(Device & device, const String & identifier,
                 const String & name)
        : Entity(device, identifier, name),
          Sensor<bool>(device, identifier, name) {}

protected:
    virtual void publish() const override {
        get_mqtt().publish(get_state_topic(), value ? "ON" : "OFF");
    }

    virtual String get_platform() const override { return "binary_sensor"; }
};

}  // namespace PicoHA
