#pragma once

#include "sensor.h"
#include "utils.h"

namespace PicoHA {

class BinarySensor : public Sensor<bool, to_string_default<bool>> {
public:
    BinarySensor(AbstractDevice & device, const String & identifier,
                 const String & name)
        : Entity(device, identifier, name),
          Sensor<bool, to_string_default<bool>>(device, identifier, name) {}

protected:
    virtual String get_platform() const override { return F("binary_sensor"); }
};

}  // namespace PicoHA
