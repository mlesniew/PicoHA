#pragma once

#include <functional>

#include "entity.h"

namespace PicoHA {

template <typename T>
class Sensor : public EntityWithState<T> {
public:
    Sensor(AbstractDevice & device, const String & identifier,
           const String & name)
        : Entity(device, identifier, name),
          EntityWithState<T>(device, identifier, name) {}

protected:
    virtual String get_platform() const override { return "sensor"; }
};

}  // namespace PicoHA